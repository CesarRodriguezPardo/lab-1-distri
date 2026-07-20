# Agentes de IA del repositorio

Tres agentes automatizan trabajo mecánico y repetitivo del repositorio,
escalando a humanos lo que requiere juicio técnico. Viven en
`.github/agents/` (scripts) y `.github/workflows/` (disparadores en CI).

## Tabla de agentes

| Agente | Herramienta | Frecuencia | Criterio mecánico | Criterio humano |
|---|---|---|---|---|
| **Documentador** (`documenter.py`) | Python 3 + reglas + Gemini 2.5 Flash (opcional) | Semanal (lunes 09:00 UTC) y al fusionar a `main` | Typo, enlace roto/vacío, sección faltante con plantilla obvia (CHANGELOG, URL del repo) → abre issue con fix sugerido | Explicar un kernel, decisiones de diseño → issue con `Requiere intervención humana: <motivo>` |
| **Revisor de bugs** (`bug_reviewer.py`) | Python 3 + reglas + Gemini 2.5 Flash (opcional) | Diaria (03:00 UTC, cron) sobre `main` | Falta `CUDA_CHECK` tras API CUDA, kernel sin `cudaGetLastError` → issue con parche sugerido | Afecta física, API pública, sincronización host/device en mediciones → issue `Requiere intervención humana` |
| **Revisor de MRs** (`mr_reviewer.py`) | Python 3 + API de GitHub + Gemini 2.5 Flash (opcional) | Al abrir/actualizar PR, **después de que CI termine** (`workflow_run`) | Solo documentación/formato/tests, CI verde, issue vinculado → comentario "mecánico y mergeable" | Cambios en kernels, física o API pública, o PR sin issue → comentario "requiere revisión humana" |

## Guardarraíles (inspirados en GitHub Agentic Workflows)

- **Permisos mínimos:** los workflows solo tienen `issues: write` o
  `pull-requests: write`. Nunca `contents: write`: los agentes no pueden
  modificar código ni fusionar a `main`.
- **Safe outputs:** las únicas escrituras permitidas son crear issues y
  comentar en PRs.
- **Los agentes nunca fusionan:** el merge a `main` siempre requiere
  aprobación humana.
- **Los agentes no reemplazan los tests:** el revisor de MRs se ejecuta
  después del pipeline de CI; si CI falla, lo dice explícitamente.
- **Límite de ruido:** máximo 5 issues automáticos por agente por semana
  (`common.py`, `MAX_AUTO_ISSUES_PER_WEEK`).
- **Sin duplicados:** no se abre un issue si ya existe uno abierto con el
  mismo título.
- **Decisión de diseño:** los agentes no abren PRs con auto-fix (el enunciado
  lo permite como opcional). Se prefirió mantenerlos de solo lectura sobre el
  código, coherente con el principio de permisos mínimos.

## Secretos y modo fallback

| Variable | Origen | ¿Obligatoria? |
|---|---|---|
| `GH_TOKEN` | `secrets.GITHUB_TOKEN` (automático de Actions) | Sí (si falta, modo dry-run local) |
| `GEMINI_API_KEY` | Secret del repositorio (Settings > Secrets) | No. Sin ella, los agentes funcionan solo con reglas |

La API key **nunca** se escribe en el código; se lee desde el entorno.

## Ejecución manual (dry-run, sin escribir en GitHub)

```bash
pip install requests
python .github/agents/documenter.py
python .github/agents/bug_reviewer.py
python .github/agents/mr_reviewer.py <numero_de_pr>
```

## Workflows

| Workflow | Disparador |
|---|---|
| `.github/workflows/agent_documenter.yml` | `schedule` semanal + `push` a `main` + manual |
| `.github/workflows/agent_bug_reviewer.yml` | `schedule` diario + manual |
| `.github/workflows/agent_mr_reviewer.yml` | `workflow_run` del workflow `CI — N-Body 2D` completado |
