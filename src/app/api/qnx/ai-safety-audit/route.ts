import { NextResponse } from 'next/server';
import { GoogleGenAI } from '@google/genai';
import { runDeterministicQnxSafetyAudit } from '@/lib/qnx-medical-engine';

export async function POST(request: Request) {
  try {
    const body = await request.json();
    const deterministicReport = runDeterministicQnxSafetyAudit({
      scenario: body.scenario || 'vtach_priority_inversion',
      heartRateBpm: Number(body.heartRateBpm ?? 188),
      mapMmhg: Number(body.mapMmhg ?? 50),
      spo2Pct: Number(body.spo2Pct ?? 86),
      qnxWcetUs: Number(body.qnxWcetUs ?? 38.5),
      linuxWcetUs: Number(body.linuxWcetUs ?? 13151.58),
      propofolHalted: Boolean(body.propofolHalted ?? true),
      hamRestartCount: Number(body.hamRestartCount ?? 1),
    });

    const apiKey = process.env.GEMINI_API_KEY;
    if (!apiKey) {
      return NextResponse.json({
        ok: true,
        report: deterministicReport,
      });
    }

    try {
      const ai = new GoogleGenAI({ apiKey });
      const prompt = `You are a QNX SDP 8.0 Embedded Medical Systems Architect and ICU Clinical Safety Officer (IEC 62304 Class C).
Provide a concise 2-sentence clinical & RTOS safety summary for:
- Patient Vitals: HR ${body.heartRateBpm} bpm, MAP ${body.mapMmhg} mmHg, SpO2 ${body.spo2Pct}%
- QNX 8.0 WCET Latency: ${body.qnxWcetUs} us (vs ${body.linuxWcetUs} us on unpartitioned general-purpose OS)
- Propofol Pump Interlock Halted: ${body.propofolHalted}
- HAM Self-Healing Driver Restarts: ${body.hamRestartCount}`;

      const response = await ai.models.generateContent({
        model: 'gemini-2.5-flash',
        contents: prompt,
      });

      const aiText = response.text?.trim();
      if (aiText) {
        deterministicReport.clinicalSbarSummary = aiText;
        deterministicReport.auditedByEngine =
          'Gemini 2.5 Flash + QNX SDP 8.0 IEC 62304 Class C Safety Auditor';
      }
    } catch {
      // Keep deterministic report if Gemini key is unavailable
    }

    return NextResponse.json({
      ok: true,
      report: deterministicReport,
    });
  } catch (err) {
    return NextResponse.json(
      { ok: false, error: err instanceof Error ? err.message : 'Safety audit failed' },
      { status: 500 }
    );
  }
}
