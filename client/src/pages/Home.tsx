/**
 * Design: Technical/Engineering Aesthetic
 * - Dark charcoal background, PCB yellow (#F5C518) accents
 * - JetBrains Mono for labels, Georgia-style serif for body
 * - Single-column, left-aligned, clean horizontal rules
 * - Green LED dot accents on section headers
 * - No animations, no decorative chrome
 */

export default function Home() {
  return (
    <div className="min-h-screen bg-[#1a1a1a] text-[#e8e0d0] font-serif">
      {/* Top rule */}
      <div className="border-t-2 border-[#F5C518]" />

      <main className="max-w-3xl mx-auto px-6 py-16">

        {/* Header */}
        <header className="mb-12">
          <p className="font-mono text-xs tracking-widest text-[#F5C518] uppercase mb-3">
            ELEC 327 — Digital Systems Laboratory — Rice University
          </p>
          <h1 className="text-4xl font-bold text-white leading-tight mb-4" style={{ fontFamily: "Georgia, 'Times New Roman', serif" }}>
            Capacitive Touch Piano
          </h1>
          <p className="font-mono text-sm text-[#999]">
            Anjali Sivasothy &amp; Eugenia Sung &nbsp;·&nbsp; May 2026
          </p>
        </header>

        <hr className="border-[#333] mb-12" />

        {/* Board Photo */}
        <section className="mb-12">
          <img
            src="https://files.manuscdn.com/user_upload_by_module/session_file/310519663336926681/ArmhbTCAHTMzUHlN.png"
            alt="Capacitive Touch Piano PCB — custom board with eight aluminum touch pads, green LEDs, MSPM0G3507 microcontroller, and MPR121 capacitive sensor"
            className="w-full rounded-sm"
            style={{ border: "1px solid #333" }}
          />
          <p className="font-mono text-xs text-[#666] mt-2">
            Custom PCB — MSPM0G3507 · MPR121 · 8-key capacitive touch array · piezo buzzer · LED indicators
          </p>
        </section>

        <hr className="border-[#333] mb-12" />

        {/* Description */}
        <section className="mb-12">
          <h2 className="font-mono text-xs tracking-widest text-[#F5C518] uppercase mb-4 flex items-center gap-2">
            <span className="inline-block w-2 h-2 rounded-full bg-[#4ade80]" />
            About the Project
          </h2>
          <div className="space-y-4 text-[#c8c0b0] leading-relaxed text-base" style={{ fontFamily: "Georgia, 'Times New Roman', serif" }}>
            <p>
              This project is a low-power embedded system that functions as a capacitive touch piano. Users play musical notes by touching copper pads on a custom printed circuit board. The system provides immediate auditory and visual feedback: a piezo buzzer sounds the corresponding note, and the LED paired with the touched key illuminates.
            </p>
            <p>
              The computational core is the Texas Instruments MSPM0G3507 microcontroller, which communicates with an NXP MPR121 capacitive touch sensor controller over a software-implemented I2C bus. Eight electrodes on the MPR121 map to eight piano keys spanning the B Major scale (B4 to B5). Additional controls include a potentiometer for volume adjustment, a button for octave shifting, and a record/playback function for saving short melodies.
            </p>
            <p>
              The entire system runs on a custom PCB designed with a mixed-signal layout strategy. Capacitive touch traces are routed separately from digital communication lines and higher-current paths to reduce noise. The microcontroller and capacitive sensor are each mounted on breakout boards that plug directly into the main PCB, eliminating dangling wires and producing a clean, self-contained device.
            </p>
          </div>
        </section>

        <hr className="border-[#333] mb-12" />

        {/* Links */}
        <section className="mb-12">
          <h2 className="font-mono text-xs tracking-widest text-[#F5C518] uppercase mb-6 flex items-center gap-2">
            <span className="inline-block w-2 h-2 rounded-full bg-[#4ade80]" />
            Resources
          </h2>
          <div className="flex flex-col gap-4 sm:flex-row sm:gap-8">
            <a
              href="https://github.com/ajs28-lgtm/elec327_final_project"
              target="_blank"
              rel="noopener noreferrer"
              className="group flex items-center gap-3 font-mono text-sm text-[#e8e0d0] hover:text-[#F5C518] transition-colors"
            >
              <svg className="w-5 h-5 flex-shrink-0" viewBox="0 0 24 24" fill="currentColor" aria-hidden="true">
                <path d="M12 2C6.477 2 2 6.477 2 12c0 4.418 2.865 8.166 6.839 9.489.5.092.682-.217.682-.482 0-.237-.009-.868-.013-1.703-2.782.604-3.369-1.342-3.369-1.342-.454-1.154-1.11-1.461-1.11-1.461-.908-.62.069-.608.069-.608 1.003.07 1.531 1.03 1.531 1.03.892 1.529 2.341 1.087 2.91.832.092-.647.35-1.088.636-1.338-2.22-.253-4.555-1.11-4.555-4.943 0-1.091.39-1.984 1.029-2.683-.103-.253-.446-1.27.098-2.647 0 0 .84-.269 2.75 1.025A9.578 9.578 0 0 1 12 6.836a9.59 9.59 0 0 1 2.504.337c1.909-1.294 2.747-1.025 2.747-1.025.546 1.377.202 2.394.1 2.647.64.699 1.028 1.592 1.028 2.683 0 3.842-2.339 4.687-4.566 4.935.359.309.678.919.678 1.852 0 1.336-.012 2.415-.012 2.743 0 .267.18.578.688.48C19.138 20.163 22 16.418 22 12c0-5.523-4.477-10-10-10z" />
              </svg>
              GitHub Repository
            </a>
            <a
              href="https://docs.google.com/presentation/d/1rfJ8ruxrcCM7E06-z038RBwosSpvh2oF1Ek66F4m0u0/edit?usp=sharing"
              target="_blank"
              rel="noopener noreferrer"
              className="group flex items-center gap-3 font-mono text-sm text-[#e8e0d0] hover:text-[#F5C518] transition-colors"
            >
              <svg className="w-5 h-5 flex-shrink-0" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" aria-hidden="true">
                <rect x="3" y="3" width="18" height="18" rx="2" />
                <path d="M8 12h8M8 8h8M8 16h5" />
              </svg>
              Project Slides
            </a>
          </div>
        </section>

        <hr className="border-[#333] mb-8" />

        {/* Footer */}
        <footer>
          <p className="font-mono text-xs text-[#555]">
            ELEC 327 Final Project &nbsp;·&nbsp; Rice University &nbsp;·&nbsp; 2026
          </p>
        </footer>

      </main>

      {/* Bottom rule */}
      <div className="border-b-2 border-[#F5C518]" />
    </div>
  );
}
