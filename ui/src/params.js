// Mirrors the ParamId enum in ~/synth/control/ParamQueue.h -- keep these in sync.

// Waveform enum shared by Osc A/B "select" controls -- values match
// engine::Waveform (Sine=0, Saw=1, Square=2, Triangle=3).
export const WAVEFORMS = [
  { value: 0, label: 'Sine' },
  { value: 1, label: 'Saw' },
  { value: 2, label: 'Square' },
  { value: 3, label: 'Triangle' },
]

// Sub oscillator only supports Sine/Square (the classic sub-bass shapes).
export const SUB_WAVEFORMS = [
  { value: 0, label: 'Sine' },
  { value: 2, label: 'Square' },
]

// Voicing mode: Poly (each note gets its own voice) vs Mono (single voice,
// legato glide between held notes).
export const VOICING_MODES = [
  { value: 0, label: 'Poly' },
  { value: 1, label: 'Mono' },
]

// Mod matrix sources/destinations, matching engine::ModSource /
// engine::ModDestination enum order.
export const MOD_SOURCES = [
  { value: 0, label: 'LFO 1' },
  { value: 1, label: 'Filter env' },
  { value: 2, label: 'Amp env' },
]
export const MOD_DESTINATIONS = [
  { value: 0, label: 'Pitch' },
  { value: 1, label: 'Filter cutoff' },
  { value: 2, label: 'Osc A level' },
  { value: 3, label: 'Osc B level' },
  { value: 4, label: 'Sub level' },
]

export const PARAMS = [
  {
    section: 'Voicing',
    controls: [
      { id: 60, label: 'Mode', type: 'select', options: VOICING_MODES, default: 0 },
      { id: 61, label: 'Glide', unit: 's', min: 0, max: 2, step: 0.01, default: 0 },
    ],
  },
  {
    section: 'Oscillator A',
    controls: [
      { id: 24, label: 'Waveform', type: 'select', options: WAVEFORMS, default: 1 },
      { id: 20, label: 'Semitone', unit: 'st', min: -24, max: 24, step: 1, default: 0 },
      { id: 21, label: 'Unison', unit: '', min: 1, max: 7, step: 1, default: 1 },
      { id: 22, label: 'Detune', unit: 'ct', min: 0, max: 50, step: 1, default: 12 },
      { id: 23, label: 'Level', unit: '', min: 0, max: 1, step: 0.01, default: 0.45 },
    ],
  },
  {
    section: 'Oscillator B',
    controls: [
      { id: 34, label: 'Waveform', type: 'select', options: WAVEFORMS, default: 2 },
      { id: 30, label: 'Semitone', unit: 'st', min: -24, max: 24, step: 1, default: 0 },
      { id: 31, label: 'Unison', unit: '', min: 1, max: 7, step: 1, default: 1 },
      { id: 32, label: 'Detune', unit: 'ct', min: 0, max: 50, step: 1, default: 12 },
      { id: 33, label: 'Level', unit: '', min: 0, max: 1, step: 0.01, default: 0.45 },
    ],
  },
  {
    section: 'Sub & Noise',
    controls: [
      { id: 41, label: 'Sub waveform', type: 'select', options: SUB_WAVEFORMS, default: 0 },
      { id: 40, label: 'Sub level', unit: '', min: 0, max: 1, step: 0.01, default: 0 },
      { id: 42, label: 'Noise level', unit: '', min: 0, max: 1, step: 0.01, default: 0.02 },
    ],
  },
  {
    section: 'Filter',
    controls: [
      { id: 0, label: 'Cutoff offset', unit: 'Hz', min: -800, max: 4000, step: 10, default: 0 },
      { id: 1, label: 'Resonance', unit: 'Q', min: 0.5, max: 8, step: 0.05, default: 0.9 },
    ],
  },
  {
    section: 'Amp envelope',
    controls: [
      { id: 2, label: 'Attack', unit: 's', min: 0.001, max: 2, step: 0.001, default: 0.005 },
      { id: 3, label: 'Decay', unit: 's', min: 0.001, max: 2, step: 0.001, default: 0.15 },
      { id: 4, label: 'Sustain', unit: '', min: 0, max: 1, step: 0.01, default: 0.7 },
      { id: 5, label: 'Release', unit: 's', min: 0.001, max: 3, step: 0.001, default: 0.3 },
    ],
  },
  {
    section: 'Filter envelope',
    controls: [
      { id: 6, label: 'Attack', unit: 's', min: 0.001, max: 2, step: 0.001, default: 0.01 },
      { id: 7, label: 'Decay', unit: 's', min: 0.001, max: 2, step: 0.001, default: 0.25 },
      { id: 8, label: 'Sustain', unit: '', min: 0, max: 1, step: 0.01, default: 0.3 },
      { id: 9, label: 'Release', unit: 's', min: 0.001, max: 3, step: 0.001, default: 0.4 },
    ],
  },
  {
    section: 'Distortion',
    controls: [
      { id: 50, label: 'Drive', unit: '', min: 1, max: 20, step: 0.1, default: 1 },
      { id: 51, label: 'Mix', unit: '', min: 0, max: 1, step: 0.01, default: 0 },
    ],
  },
  {
    section: 'Delay',
    controls: [
      { id: 52, label: 'Time', unit: 'ms', min: 0, max: 2000, step: 10, default: 250 },
      { id: 53, label: 'Feedback', unit: '', min: 0, max: 0.95, step: 0.01, default: 0.3 },
      { id: 54, label: 'Mix', unit: '', min: 0, max: 1, step: 0.01, default: 0 },
    ],
  },
  {
    section: 'Mod matrix',
    controls: [
      { id: 70, label: 'Slot 1 source', type: 'dropdown', options: MOD_SOURCES, default: 0 },
      { id: 71, label: 'Slot 1 dest', type: 'dropdown', options: MOD_DESTINATIONS, default: 0 },
      { id: 72, label: 'Slot 1 amount', unit: '', min: -1, max: 1, step: 0.01, default: 0 },
      { id: 80, label: 'Slot 2 source', type: 'dropdown', options: MOD_SOURCES, default: 1 },
      { id: 81, label: 'Slot 2 dest', type: 'dropdown', options: MOD_DESTINATIONS, default: 1 },
      { id: 82, label: 'Slot 2 amount', unit: '', min: -1, max: 1, step: 0.01, default: 0 },
    ],
  },
  {
    section: 'LFO',
    controls: [
      { id: 10, label: 'LFO rate', unit: 'Hz', min: 0.1, max: 20, step: 0.1, default: 4.5 },
      { id: 11, label: 'LFO -> cutoff', unit: 'Hz', min: -1000, max: 1000, step: 10, default: 150 },
      { id: 12, label: 'LFO -> pitch', unit: 'st', min: -2, max: 2, step: 0.01, default: 0.06 },
      { id: 13, label: 'Env2 -> cutoff', unit: 'Hz', min: -6000, max: 6000, step: 50, default: 3500 },
    ],
  },
]

