# Flujo de trabajo Git — Laboratorio 2

Este documento describe el flujo Git profesional del repositorio, según la
sección 6 del enunciado del Laboratorio 2.

## 1. Rama protegida

`main` está protegida en GitHub (Settings > Branches):

- ❌ Sin push directo (incluye administradores).
- ✅ Merge solo vía Pull Request.
- ✅ Al menos **1 revisión humana** del equipo antes de fusionar.
- ✅ **CI verde** requerido (`CI — N-Body 2D`: compilación + `make test`).
- ✅ Ramas actualizadas antes de mergear.
- ✅ Conversaciones resueltas antes de mergear.
- ✅ Eliminación automática de ramas fusionadas.

## 2. Convención de ramas

| Tipo | Patrón | Uso |
|---|---|---|
| Funcionalidad | `feature/<nombre>` | Kernels, agentes, CHANGELOG, etc. |
| Corrección | `fix/<nombre>` | Bugs en código, tests o CI |

## 3. Commits convencionales

```
<tipo>(<scope>): <descripción corta>
```

Tipos: `feat`, `fix`, `docs`, `test`, `ci`, `chore`.
Scopes sugeridos: `cuda`, `memory`, `testing`, `ci`, `agents`, `git`, `docs`.

Ejemplo: `feat(cuda): add shared-memory tile kernel`

## 4. Regla de vinculación

Todo PR debe referenciar al menos un issue:

- `Closes #N` — el PR resuelve el issue (se cierra al fusionar).
- `Refs #N` — el PR está relacionado con el issue.

**Los PRs sin issue asociado pueden penalizarse en la rúbrica de Git.**

## 5. Flujo paso a paso

```bash
git checkout main && git pull
git checkout -b feature/<nombre>
# ... commits convencionales ...
git push -u origin feature/<nombre>
# Abrir PR: "<tipo>(<scope>): <título> (Closes #N)"
# CI pasa -> agente MR comenta -> revisión humana -> merge
# La rama se elimina automáticamente tras el merge
```

## 6. Issues

- Los crea el equipo (no el docente): mínimo 5 durante el laboratorio,
  al menos 1 por rol distinto.
- Cada issue lleva: título claro, descripción, etiquetas y asignado.
- El rol de Git, releases y agentes coordina el tablero.

## 7. Releases

- `CHANGELOG.md` sigue el formato [Keep a Changelog](https://keepachangelog.com/).
- Al entregar: tag `v2.0.0-lab2` en `main` con notas de release que resumen
  cambios CUDA, CI y agentes.

## 8. Agentes de IA

Tres agentes automatizan documentación, revisión de bugs y revisión de PRs.
Viven en `.github/agents/` y `.github/workflows/`. Documentación completa en
[`.github/agents/README.md`](agents/README.md).

**Los agentes nunca fusionan a `main`.** Toda escritura suya es un issue o un
comentario; el juicio final siempre es humano.
