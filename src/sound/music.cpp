#ifndef NO_SDL
#include <SDL_mixer.h>
/// #include <SDL.h>

#include <cstdio>

#include "music.h"

// The audio device is opened lazily and kept for the lifetime of the process.
// Opening and closing it on every call was wasteful, and loading the cue only
// to free it again meant nothing was ever actually playing.
static bool audio_ready = false;
static Mix_Chunk* cue = nullptr;

void play_music(MidiFile midi) {
  (void)midi;  // TODO: pick a different cue per MidiFile value

  if (!audio_ready) {
    if (Mix_OpenAudio(48000, AUDIO_S16SYS, 2, 512) != 0) {
      // TODO: determine why sporadic static noise occurs when set to 44100
      std::fprintf(stderr, "Mix_OpenAudio failed: %s\n", Mix_GetError());
      return;
    }
    Mix_AllocateChannels(4);
    audio_ready = true;
  }

  if (cue == nullptr) {
    cue = Mix_LoadWAV("test.wav");
    if (cue == nullptr) {
      return;  // TODO: the cue asset is not bundled yet
    }
  }

  // TODO: mix the cue in.
///   Mix_PlayChannel(-1, cue, 0);
}
#endif
