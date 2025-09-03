
#if defined(PLATFORM_WEB)
// ---------------- WEB AUDIO SHIM (slots map to your Song_* enum) ----------------
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

// The music slots operate like
// slot 0 = Song_intro
// slot 1 = Song_play 
// slot 2 = Song_play_muted
// I can add more slots later and the sound effects 
// can be done in the same way.

static const char *g_song_paths[Song_count] = {
    "../assets/sounds/music.wav",          // Song_play
    "../assets/sounds/music_muted.wav",    // Song_play_muted
    "../assets/sounds/tutorial_track.wav", // Song_tutorial
    "../assets/sounds/intro_music.wav",    // Song_intro
    "../assets/sounds/win_track.wav",      // Song_win
};

static const char *g_sfx_paths[SoundEffect_count] = {
    "../assets/sounds/powerup.wav",         // SoundEffect_powerup
    "../assets/sounds/powerup_end.wav",     // SoundEffect_powerup_end
    "../assets/sounds/powerup_collect.wav", // SoundEffect_powerup_collect
    "../assets/sounds/powerup_appear.wav",  // SoundEffect_powerup_appear
    "../assets/sounds/start.wav",           // SoundEffect_spacebar
};

static const char *g_hype_paths[HYPE_WORD_COUNT] = {
    "../assets/sounds/hype_1.wav", 
    "../assets/sounds/hype_2.wav", 
    "../assets/sounds/hype_3.wav", 
    "../assets/sounds/hype_4.wav", 
    "../assets/sounds/hype_5.wav", 
    "../assets/sounds/hype_6.wav", 
    "../assets/sounds/hype_7.wav", 
    "../assets/sounds/hype_8.wav", 
    "../assets/sounds/hype_9.wav", 
    "../assets/sounds/hype_10.wav", 
    "../assets/sounds/hype_11.wav", 
    "../assets/sounds/hype_12.wav", 
};

EM_JS(void, wa_setup, (), {
  if (!Module._wa) Module._wa = {};
  const A = Module._wa;
  A.ctx = A.ctx || new (window.AudioContext || window.webkitAudioContext)();
  if (A.ctx.state === 'suspended') A.ctx.resume();

  A.master = A.master || A.ctx.createGain(); A.master.gain.value = 1.0;
  A.master.connect(A.ctx.destination);

  // per-slot state
  if (!A.slots) {
    A.slots = {};
    for (let i = 0; i < 8; ++i) {
      A.slots[i] = { gain: null, src: null, html: null, startTime: 0, duration: 0, elNode: null };
    }
  }
  for (let i = 0; i < 8; ++i) {
    const S = A.slots[i];
    if (!S.gain) { S.gain = A.ctx.createGain(); S.gain.gain.value = 0.0; S.gain.connect(A.master); }
  }
});

EM_JS(void, wa_unlock, (), {
  const A = Module._wa; if (!A || !A.ctx) return;
  if (A.ctx.state === 'suspended') A.ctx.resume();
  // nudge pull
  const o = A.ctx.createOscillator(); const g = A.ctx.createGain(); g.gain.value = 0.0;
  o.connect(g).connect(A.master); o.start(); o.stop(A.ctx.currentTime + 0.01);
});

EM_JS(int, wa_is_unlocked, (), {
  const A = Module._wa; return (A && A.ctx && A.ctx.state === 'running') ? 1 : 0;
});

static inline bool Web_IsUnlocked() { 
    return wa_is_unlocked() != 0; 
}

EM_JS(void, wa_slot_stop, (int slot), {
  const A = Module._wa; if (!A) return; const S = A.slots[slot]; if (!S) return;
  try { if (S.src) S.src.stop(); } catch(e) {}
  S.src = null;
  if (S.html) { try { S.html.pause(); } catch(e) {} S.html = null; }
});

EM_JS(void, wa_slot_set_volume, (int slot, double v), {
  const A = Module._wa; if (!A) return; const S = A.slots[slot]; if (!S || !S.gain) return;
  S.gain.gain.value = Math.max(0, v);
});

EM_JS(int, wa_slot_is_playing, (int slot), {
  const A = Module._wa; if (!A) return 0; const S = A.slots[slot]; if (!S) return 0;
  if (S.html) return !S.html.paused ? 1 : 0;
  return S.src ? 1 : 0;
});

// Read file BYTES from MEMFS, decode via WebAudio; fallback to HTMLAudio(Blob) if decode fails.
EM_JS(void, wa_slot_play_file, (int slot, const char* path_c, int loop), {
  try {
    const path = UTF8ToString(path_c);
    const A = Module._wa; if (!A) return;
    if (!A.ctx) { Module._wa = {}; wa_setup(); }
    if (A.ctx.state === 'suspended') A.ctx.resume();

    const S = A.slots[slot]; if (!S) return;
    // Stop anything currently playing
    try { if (S.src) S.src.stop(); } catch(e) {}
    if (S.html) { try { S.html.pause(); } catch(e) {} S.html = null; }
    S.src = null;

    // Read from MEMFS (because --preload-file is used in the web build script)
    const u8 = FS.readFile(path);
    const ab = u8.buffer.slice(u8.byteOffset, u8.byteOffset + u8.byteLength);

    function mime(p){
      const ext = p.split('.').pop().toLowerCase();
      if (ext==='mp3') return 'audio/mpeg';
      if (ext==='ogg'||ext==='oga') return 'audio/ogg';
      if (ext==='wav') return 'audio/wav';
      return 'application/octet-stream';
    }

    A.ctx.decodeAudioData(ab.slice(0)).then(buf => {
      const node = A.ctx.createBufferSource();
      node.buffer = buf;
      node.loop = !!loop;
      node.connect(S.gain);
      node.start();
      S.src = node;
      S.startTime = A.ctx.currentTime;
      S.duration  = buf.duration;
      S.html = null;
    }).catch(err => {
      console.warn('decodeAudioData failed; falling back to HTMLAudioElement:', err);
      const blob = new Blob([ab], { type: mime(path) });
      const url  = URL.createObjectURL(blob);
      const el   = new Audio();
      el.src   = url;
      el.loop  = !!loop;
      el.volume = 1.0;  // volume is controlled by S.gain if routed
      // Pipe element → graph if allowed
      try {
        if (!S.elNode && A.ctx.createMediaElementSource) {
          S.elNode = A.ctx.createMediaElementSource(el);
          S.elNode.connect(S.gain);
        }
      } catch(e) {}
      el.play().then(()=> {
        S.html = el;
        el.addEventListener('loadedmetadata', ()=> { S.duration = isNaN(el.duration)?0:el.duration; }, { once:true });
      }).catch(e => console.error('element.play() failed', e));
    });

  } catch(e) { console.error('wa_slot_play_file error', e); }
});

EM_JS(void, wa_sfx_init, (), {
  if (!Module._wa) Module._wa = {};
  const A = Module._wa;
  A.ctx = A.ctx || new (window.AudioContext || window.webkitAudioContext)();
  if (A.ctx.state === 'suspended') A.ctx.resume();

  A.master = A.master || A.ctx.createGain();
  A.master.gain.value = 1.0;
  A.master.connect(A.ctx.destination);

  if (!A.sfx) A.sfx = { buffers:{}, gains:{}, actives:{}, master: null };
  if (!A.sfx.master) { A.sfx.master = A.ctx.createGain(); A.sfx.master.gain.value = 1.0; A.sfx.master.connect(A.master); }
});

// WEB AUDIO SFX
EM_JS(void, wa_sfx_preload, (int id, const char* path_c), {
  try {
    const path = UTF8ToString(path_c);
    const A = Module._wa; if (!A) return;
    if (!A.ctx) { Module._wa = {}; wa_sfx_init(); }
    if (A.ctx.state === 'suspended') A.ctx.resume();

    if (!A.sfx) wa_sfx_init();
    if (!A.sfx.gains[id]) { const g = A.sfx.gains[id] = A.ctx.createGain(); g.gain.value = 1.0; g.connect(A.sfx.master); }
    if (!A.sfx.actives[id]) A.sfx.actives[id] = [];

    // Ensure the file exists in MEMFS
    const info = FS.analyzePath(path);
    if (!info.exists) { console.error('SFX MEMFS missing:', path); return; }

    const u8 = FS.readFile(path);
    const ab = u8.buffer.slice(u8.byteOffset, u8.byteOffset + u8.byteLength);

    A.ctx.decodeAudioData(ab.slice(0)).then(buf => {
      A.sfx.buffers[id] = buf;
      // console.log('SFX decoded', id, path, buf.duration);
    }).catch(err => {
      console.error('SFX decode failed', path, err);
    });
  } catch(e) { console.error('wa_sfx_preload error', e); }
});

EM_JS(void, wa_sfx_set_volume, (int id, double v), {
  const A = Module._wa; if (!A || !A.sfx || !A.sfx.gains[id]) return;
  A.sfx.gains[id].gain.value = Math.max(0, v);
});

EM_JS(void, wa_sfx_play, (int id), {
  const A = Module._wa; if (!A || !A.sfx) return;
  if (A.ctx.state === 'suspended') A.ctx.resume();
  const buf = A.sfx.buffers[id]; if (!buf) return; // not loaded yet
  const g = A.sfx.gains[id];     if (!g) return;

  const src = A.ctx.createBufferSource();
  src.buffer = buf;
  src.connect(g);
  src.start();
  if (!A.sfx.actives[id]) A.sfx.actives[id] = [];
  A.sfx.actives[id].push(src);
  src.onended = () => {
    const arr = A.sfx.actives[id]; if (!arr) return;
    const i = arr.indexOf(src); if (i >= 0) arr.splice(i, 1);
  };
});

EM_JS(int, wa_sfx_is_playing, (int id), {
  const A = Module._wa; 
  if (!A || !A.sfx || !A.sfx.actives[id]) return 0;
  return A.sfx.actives[id].length > 0 ? 1 : 0;
});

EM_JS(void, wa_sfx_stop, (int id), {
  const A = Module._wa; if (!A || !A.sfx || !A.sfx.actives[id]) return;
  const arr = A.sfx.actives[id].slice();
  arr.forEach(src => { try { src.stop(); } catch(e){} });
  A.sfx.actives[id] = [];
});

EM_JS(void, wa_sfx_stop_all, (), {
  const A = Module._wa; if (!A || !A.sfx) return;
  for (const k in A.sfx.actives) {
    const arr = A.sfx.actives[k].slice();
    arr.forEach(src => { try { src.stop(); } catch(e){} });
    A.sfx.actives[k] = [];
  }
});

static inline void WebAudioInit() { wa_setup(); }
static inline void WebAudioUnlockOnGesture() { wa_setup(); wa_unlock(); }
static inline void WebAudioPlaySlot(int slot, const char *path, bool loop) { wa_slot_play_file(slot, path, loop?1:0); }
static inline void WebAudioStopSlot(int slot) { wa_slot_stop(slot); }
static inline void WebAudioSetVol(int slot, float v) { wa_slot_set_volume(slot, (double)v); }
static inline bool WebAudioIsPlaying(int slot) { return wa_slot_is_playing(slot) != 0; }

static inline void WebAudioSfxInit() { wa_sfx_init(); }
static inline void WebAudioSfxPreload(int id, const char *path) { wa_sfx_preload(id, path); }
static inline void WebAudioSfxSetVolume(int id, float v) { wa_sfx_set_volume(id, (double)v); }
static inline void WebAudioSfxPlay(int id) { wa_sfx_play(id); }
static inline bool WebAudioSfxIsPlaying(int id) { return wa_sfx_is_playing(id) != 0; }
static inline void WebAudioSfxStop(int id) { wa_sfx_stop(id); }
static inline void WebAudioSfxStopAll() { wa_sfx_stop_all(); }
#endif

