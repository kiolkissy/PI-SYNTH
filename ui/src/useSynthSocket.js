import { useEffect, useRef, useState } from 'react'

// Connects to the synth's ControlServer and exposes a `sendParam` function.
// Auto-reconnects since the WS server may not be up yet, or the engine
// may restart during development.
export function useSynthSocket() {
  const [connected, setConnected] = useState(false)
  const wsRef = useRef(null)
  const waveformListeners = useRef(new Set())
  const noteListeners = useRef(new Set())

  useEffect(() => {
    let cancelled = false
    let retryTimer = null

    function connect() {
      if (cancelled) return
      const url = `ws://${window.location.hostname}:9002`
      const ws = new WebSocket(url)
      wsRef.current = ws

      ws.onopen = () => setConnected(true)
      ws.onclose = () => {
        setConnected(false)
        if (!cancelled) retryTimer = setTimeout(connect, 2000)
      }
      ws.onerror = () => ws.close()
      ws.onmessage = (event) => {
        let msg
        try {
          msg = JSON.parse(event.data)
        } catch {
          return
        }
        if (msg.type === 'waveform' && Array.isArray(msg.samples)) {
          for (const listener of waveformListeners.current) listener(msg.samples)
        }
        if (msg.type === 'noteOn' || msg.type === 'noteOff') {
          for (const listener of noteListeners.current) listener(msg.type, msg.note)
        }
      }
    }

    connect()
    return () => {
      cancelled = true
      clearTimeout(retryTimer)
      wsRef.current?.close()
    }
  }, [])

  function sendParam(paramId, value) {
    const ws = wsRef.current
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({ param: paramId, value }))
    }
  }

  function sendNoteOn(note, velocity = 0.85) {
    const ws = wsRef.current
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({ type: 'noteOn', note, velocity }))
    }
  }

  function sendNoteOff(note) {
    const ws = wsRef.current
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({ type: 'noteOff', note }))
    }
  }

  // Subscribes to live waveform sample arrays pushed from the engine.
  // Returns an unsubscribe function. Kept out of React state since updates
  // arrive at ~30Hz and are better drawn directly to a canvas.
  function onWaveform(listener) {
    waveformListeners.current.add(listener)
    return () => waveformListeners.current.delete(listener)
  }

  // Subscribes to note-on/off events echoed back from the engine -- used to
  // highlight notes played on real MIDI hardware on the onscreen keyboard.
  // Returns an unsubscribe function.
  function onNoteEvent(listener) {
    noteListeners.current.add(listener)
    return () => noteListeners.current.delete(listener)
  }

  return { connected, sendParam, sendNoteOn, sendNoteOff, onWaveform, onNoteEvent }
}
