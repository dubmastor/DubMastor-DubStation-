# RiddimBank — banco único de riddims da Dub Station

Fonte única dos riddims que o MRCM-1240, o MRBM-69, o MRDM-12 e o MRM-0420 usam.
**Ainda não está ligado a nenhum plugin**: os bancos de cada máquina continuam como estão até a integração ser aprovada.

| Arquivo | O que é |
| --- | --- |
| `riddims.json` | o banco (gerado — não editar à mão) |
| `CATALOGO.md` | o catálogo para ler, por estilo |
| `riddims.csv` | o mesmo banco em tabela |
| `correcoes.json` | correções de harmonia, músicas para juntar, riddim de cada música e riddims novos (com a fonte de cada um) |
| `tools/build_bank.py` | gera os três arquivos acima a partir dos bancos das máquinas + `correcoes.json` |

Uso: `python3 RiddimBank/tools/build_bank.py` (na raiz do repositório).

## Regras
- **Uma entrada por gravação.** O campo `riddim` liga músicas gravadas no mesmo riddim.
- **Harmonia:** correção pesquisada > MRCM-1240 (cifra feita por pessoa) > MRBM-69 > MRCM-1240 `[auto]` > MRBM-69 `~`.
  `harmoniaStatus`: `conferida`, `aproximada` (detecção automática ou de ouvido) ou `pendente`.
- **BPM** contado como a bateria do MRDM-12 (caixa no 2 e no 4, nunca acima de 100): medido na gravação > groove do MRDM-12 > MRBM-69 > típico da época.
- **Bateria:** o groove original quando o MRDM-12 tem a música; senão, um groove escolhido do baterista mais próximo de quem gravou (mesmo estilo de bateria, BPM e época mais próximos).
- **Baixo:** o riddim original quando o MRBM-69 tem a música; senão, o estilo do baixista mais próximo de quem gravou, com densidade, variação, swing e timbre da época.
- **Índices** (`indices.MRCM`, `.MRBM`, `.MRDM`): posição nos bancos atuais. A ordem desses bancos não muda, porque o índice fica salvo nos projetos.
