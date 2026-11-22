# Pro Tools (AAX) nos plugins DubMastor

O Pro Tools só abre plugins no formato **AAX**, e o Pro Tools normal (o que os usuários têm) só carrega AAX **assinado pela PACE**, com autorização da Avid. Isso não dá para resolver só com código: precisa das contas abaixo. O repositório já está pronto — quando os secrets existirem, o workflow `build-pacote.yml` compila, assina e coloca o AAX no instalador completo sozinho.

## O que já está feito
- Todos os plugins compilam em AAX no Windows e no Mac (`.github/cmake/aax.cmake`), com o SDK do AAX que já vem no JUCE 8.
- O instalador completo instala o AAX em `C:\Program Files\Common Files\Avid\Audio\Plug-Ins` (Windows) e `/Library/Application Support/Avid/Audio/Plug-Ins` (Mac), com a opção "Pro Tools (AAX)".
- AAX compilado mas sem assinatura **não** entra no instalador público: vai para o artefato `AAX-nao-assinado-<sistema>` do workflow, para testar no Pro Tools Developer.

## Passo a passo (uma vez só)
1. **Conta de desenvolvedor Avid** (gratuita): cadastre-se em developer.avid.com e baixe o **Pro Tools Developer** (versão de teste que abre AAX sem assinatura). O SDK não precisa: já vem no JUCE 8.
2. **PACE / iLok**: peça à Avid o acesso ao programa de assinatura de AAX. A PACE manda o **Eden SDK** (programa `wraptool`), uma conta e um **WCGUID** (o ID da assinatura).
3. **Certificado de assinatura**:
   - Windows: um certificado de assinatura de código `.p12` (a PACE aceita um certificado próprio gerado para isso).
   - Mac: um certificado **Developer ID Application** da Apple (conta Apple Developer, US$ 99/ano), exportado como `.p12`.
4. Guarde o `wraptool` num link privado (por exemplo, numa release de um repositório privado).
5. Em **Settings → Secrets and variables → Actions** do repositório, crie:

| Secret | O que é |
|---|---|
| `PACE_TOOLS_URL` | link do zip com o `wraptool` (Windows e Mac) |
| `PACE_USERNAME` / `PACE_PASSWORD` | conta da PACE |
| `PACE_WCGUID` | ID da assinatura que a PACE enviou |
| `PACE_KEYFILE_B64` / `PACE_KEYPASSWORD` | certificado `.p12` do Windows em base64 e a senha |
| `MAC_CERT_P12_B64` / `MAC_CERT_PASSWORD` | certificado Developer ID do Mac em base64 e a senha |
| `MAC_SIGN_ID` | nome do certificado, ex.: `Developer ID Application: Seu Nome (XXXXXXXXXX)` |

6. Rode o workflow **Build Dub Station — instalador completo** (Actions → Run workflow). Sem os secrets ele já gera o AAX para testar no Pro Tools Developer (artefato `AAX-nao-assinado-*`); com eles, o AAX assinado entra no instalador.
7. Depois de testar no Pro Tools, suba a versão de cada plugin (regra do repositório), troque no site a linha do Pro Tools na tabela de DAWs e o texto dos botões para "VST3 + AAX".
