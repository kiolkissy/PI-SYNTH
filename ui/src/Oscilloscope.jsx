import { useEffect, useRef } from 'react'

// Live canvas oscilloscope: draws whatever raw audio samples the engine is
// actually outputting, redrawn continuously via requestAnimationFrame. This
// exists so you can visually confirm the synth is generating sound even
// when there's no speaker/audio interface connected to hear it.
export function Oscilloscope({ onWaveform, connected }) {
  const canvasRef = useRef(null)
  const latestSamplesRef = useRef(null)
  const lastReceivedAtRef = useRef(0)

  useEffect(() => {
    const unsubscribe = onWaveform((samples) => {
      latestSamplesRef.current = samples
      lastReceivedAtRef.current = performance.now()
    })
    return unsubscribe
  }, [onWaveform])

  useEffect(() => {
    const canvas = canvasRef.current
    const ctx = canvas.getContext('2d')
    let rafId = null

    function draw() {
      const dpr = window.devicePixelRatio || 1
      const width = canvas.clientWidth
      const height = canvas.clientHeight
      const targetW = Math.round(width * dpr)
      const targetH = Math.round(height * dpr)
      if (canvas.width !== targetW || canvas.height !== targetH) {
        canvas.width = targetW
        canvas.height = targetH
      }
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0)
      ctx.clearRect(0, 0, width, height)

      // Center baseline.
      ctx.strokeStyle = '#2a2d37'
      ctx.lineWidth = 1
      ctx.beginPath()
      ctx.moveTo(0, height / 2)
      ctx.lineTo(width, height / 2)
      ctx.stroke()

      const isStale = performance.now() - lastReceivedAtRef.current > 500
      const samples = isStale ? null : latestSamplesRef.current

      if (samples && samples.length > 1) {
        ctx.strokeStyle = '#33e07a'
        ctx.lineWidth = 1.5
        ctx.shadowColor = 'rgba(51, 224, 122, 0.55)'
        ctx.shadowBlur = 4
        ctx.beginPath()
        const midY = height / 2
        const amp = height / 2 - 4
        for (let i = 0; i < samples.length; i++) {
          const x = (i / (samples.length - 1)) * width
          const y = midY - samples[i] * amp
          if (i === 0) ctx.moveTo(x, y)
          else ctx.lineTo(x, y)
        }
        ctx.stroke()
        ctx.shadowBlur = 0
      }

      rafId = requestAnimationFrame(draw)
    }

    rafId = requestAnimationFrame(draw)
    return () => cancelAnimationFrame(rafId)
  }, [])

  const showNoSignal = !connected
  return (
    <div className="scope-panel">
      <div className="scope-head">
        <span className="scope-title">Output</span>
        <span className="scope-hint">live waveform from the audio engine</span>
      </div>
      <div className="scope-canvas-wrap">
        <canvas ref={canvasRef} className="scope-canvas" />
        {showNoSignal && <div className="scope-overlay">Engine offline &mdash; no signal</div>}
      </div>
    </div>
  )
}
