import { useCallback, useEffect, useRef, useState } from 'react'

const NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
const WHITE_SEMITONES = new Set([0, 2, 4, 5, 7, 9, 11])

// 5 octaves plus the top C, matching a standard 61-key keyboard span.
const LOWEST_NOTE = 36 // C2
const HIGHEST_NOTE = 96 // C7

// QWERTY row mapped to one octave (semitone offsets from a shiftable base
// note), so the synth is fully playable from a computer keyboard when
// there's no MIDI controller or touchscreen handy.
const KEY_MAP = {
  KeyA: 0,
  KeyW: 1,
  KeyS: 2,
  KeyE: 3,
  KeyD: 4,
  KeyF: 5,
  KeyT: 6,
  KeyG: 7,
  KeyY: 8,
  KeyH: 9,
  KeyU: 10,
  KeyJ: 11,
  KeyK: 12,
}

function buildKeys() {
  const keys = []
  let whiteCountSoFar = 0
  for (let note = LOWEST_NOTE; note <= HIGHEST_NOTE; note++) {
    const semitone = note % 12
    const isWhite = WHITE_SEMITONES.has(semitone)
    const octave = Math.floor(note / 12) - 1
    // For black keys, this is how many white keys come before it -- used to
    // position it at that boundary in the white-key row below.
    keys.push({ note, isWhite, name: `${NOTE_NAMES[semitone]}${octave}`, whiteIndexBefore: whiteCountSoFar })
    if (isWhite) whiteCountSoFar++
  }
  return keys
}

const KEYS = buildKeys()
const WHITE_KEYS = KEYS.filter((k) => k.isWhite)
const BLACK_KEYS = KEYS.filter((k) => !k.isWhite)

export function Keyboard({ onNoteOn, onNoteOff, onNoteEvent }) {
  const [baseNote, setBaseNote] = useState(60) // C4
  const [activeNotes, setActiveNotes] = useState(() => new Set())
  const pointerNotes = useRef(new Map()) // pointerId -> note, for glissando-safe release
  const computerNotes = useRef(new Set())

  const activate = useCallback((note) => {
    setActiveNotes((prev) => (prev.has(note) ? prev : new Set(prev).add(note)))
  }, [])
  const deactivate = useCallback((note) => {
    setActiveNotes((prev) => {
      if (!prev.has(note)) return prev
      const next = new Set(prev)
      next.delete(note)
      return next
    })
  }, [])

  const noteOn = useCallback(
    (note) => {
      onNoteOn(note)
      activate(note)
    },
    [onNoteOn, activate],
  )
  const noteOff = useCallback(
    (note) => {
      onNoteOff(note)
      deactivate(note)
    },
    [onNoteOff, deactivate],
  )

  // Highlight notes played from real MIDI hardware or another client, echoed
  // back over the WebSocket -- keeps the onscreen keyboard in sync with
  // whatever is actually sounding, not just its own local presses.
  useEffect(() => {
    if (!onNoteEvent) return undefined
    return onNoteEvent((type, note) => {
      if (type === 'noteOn') activate(note)
      else deactivate(note)
    })
  }, [onNoteEvent, activate, deactivate])

  function handlePointerDown(e, note) {
    e.preventDefault()
    e.currentTarget.setPointerCapture(e.pointerId)
    pointerNotes.current.set(e.pointerId, note)
    noteOn(note)
  }

  function handlePointerUp(e) {
    const note = pointerNotes.current.get(e.pointerId)
    if (note !== undefined) {
      noteOff(note)
      pointerNotes.current.delete(e.pointerId)
    }
  }

  useEffect(() => {
    function handleKeyDown(e) {
      if (e.repeat || e.target instanceof HTMLInputElement) return
      if (!(e.code in KEY_MAP)) return
      const note = baseNote + KEY_MAP[e.code]
      if (computerNotes.current.has(note)) return
      computerNotes.current.add(note)
      noteOn(note)
    }

    function handleKeyUp(e) {
      if (e.code === 'KeyZ') {
        setBaseNote((n) => Math.max(LOWEST_NOTE, n - 12))
        return
      }
      if (e.code === 'KeyX') {
        setBaseNote((n) => Math.min(HIGHEST_NOTE - 12, n + 12))
        return
      }
      if (!(e.code in KEY_MAP)) return
      const note = baseNote + KEY_MAP[e.code]
      computerNotes.current.delete(note)
      noteOff(note)
    }

    window.addEventListener('keydown', handleKeyDown)
    window.addEventListener('keyup', handleKeyUp)
    return () => {
      window.removeEventListener('keydown', handleKeyDown)
      window.removeEventListener('keyup', handleKeyUp)
    }
  }, [baseNote, noteOn, noteOff])

  const baseOctaveLabel = `${NOTE_NAMES[baseNote % 12]}${Math.floor(baseNote / 12) - 1}`

  return (
    <div className="keyboard-panel">
      <div className="keyboard-head">
        <span className="keyboard-title">Onscreen keyboard</span>
        <span className="keyboard-hint">
          Click/tap to play &middot; computer keys A W S E D F T G Y H U J K play from {baseOctaveLabel}{' '}
          &middot; Z / X shifts octave
        </span>
      </div>
      <div className="keyboard-strip" style={{ '--white-count': WHITE_KEYS.length }}>
        <div className="white-keys">
          {WHITE_KEYS.map((k) => (
            <div
              key={k.note}
              className={`key white${activeNotes.has(k.note) ? ' active' : ''}`}
              onPointerDown={(e) => handlePointerDown(e, k.note)}
              onPointerUp={handlePointerUp}
              onPointerLeave={handlePointerUp}
              onPointerCancel={handlePointerUp}
            >
              {k.name.startsWith('C') && !k.name.startsWith('C#') && (
                <span className="key-label">{k.name}</span>
              )}
            </div>
          ))}
        </div>
        <div className="black-keys">
          {BLACK_KEYS.map((k) => (
            <div
              key={k.note}
              className={`key black${activeNotes.has(k.note) ? ' active' : ''}`}
              style={{ '--white-index': k.whiteIndexBefore }}
              onPointerDown={(e) => handlePointerDown(e, k.note)}
              onPointerUp={handlePointerUp}
              onPointerLeave={handlePointerUp}
              onPointerCancel={handlePointerUp}
            />
          ))}
        </div>
      </div>
    </div>
  )
}
