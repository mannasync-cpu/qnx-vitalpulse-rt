import type { Metadata } from 'next';
import './globals.css';

export const metadata: Metadata = {
  title: 'QNX VitalPulse RT — Deterministic ICU & Closed-Loop Infusion Safety Microkernel (QNX SDP 8.0)',
  description:
    'IEC 62304 Class C Real-Time ICU Bedside Monitor, Priority-Inherited Closed-Loop Infusion Interlock, Adaptive Partitioning (APS) & High-Availability Manager (HAM) Workbench for QNX SDP 8.0.',
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="en" className="dark">
      <body className="min-h-screen bg-[#050811] text-slate-100 antialiased">
        {children}
      </body>
    </html>
  );
}
