# DrawaBattle GBA

Protótipo homebrew para Game Boy Advance, inspirado no jogo web que você me
mandou (desenhar um símbolo para lançar magia + sobreviver a ondas de
inimigos). Código-fonte completo em C, escrito direto contra os registradores
do hardware (sem depender de libgba além do link final), modo gráfico 3
(bitmap 240x160).

Não tenho o compilador ARM (devkitARM) disponível aqui no meu ambiente para
gerar o `.gba` já pronto — então te entrego o projeto completo pra você
compilar localmente. É rápido.

## 1. Instalar o devkitPro (uma vez só)

- **Windows/macOS/Linux**: baixe o instalador em https://devkitpro.org/wiki/Getting_Started
- Durante a instalação, marque o pacote **gba-dev** (ou rode depois):
  ```
  (sudo) dkp-pacman -S gba-dev
  ```
- Defina a variável de ambiente (ajuste o caminho se for diferente):
  ```
  export DEVKITARM=/opt/devkitpro/devkitARM        # Linux/macOS
  set DEVKITARM=C:\devkitPro\devkitARM             # Windows (cmd)
  ```

## 2. Compilar

Dentro da pasta do projeto:
```
make
```
Isso gera `drawabattle.gba` na raiz do projeto.

## Compilando sem PC (só com o celular, via Termux + GitHub Actions)

devkitPro não roda no Android/Termux (é feito pra Windows/Linux/macOS), então
compilar direto no celular com o toolchain oficial não é confiável. O
caminho que funciona 100% do celular: usar o Termux só pra mandar o código
pro GitHub, e deixar o **GitHub Actions** (servidor de build gratuito da
Microsoft/GitHub, na nuvem) compilar pra você usando a imagem oficial do
devkitPro. O projeto já vem com o workflow pronto em
`.github/workflows/build.yml`.

1. No navegador do celular, crie uma conta no https://github.com (se não
   tiver) e crie um repositório novo, vazio, público ou privado — o nome não
   importa, ex: `drawabattle-gba`.
2. Gere um **token de acesso**: Configurações da conta → Developer settings →
   Personal access tokens → Generate new token (classic) → marque o escopo
   `repo` → gera e **copia o token** (só aparece uma vez, salve em algum
   lugar seguro tipo o gerenciador de senhas).
3. No Termux:
   ```
   pkg update && pkg install git
   cd ~/storage/downloads      # ou onde o .zip do projeto estiver
   unzip drawabattle_gba.zip
   cd drawabattle_gba
   git init
   git add .
   git commit -m "primeiro commit"
   git branch -M main
   git remote add origin https://github.com/SEU_USUARIO/drawabattle-gba.git
   git push -u origin main
   ```
   Se não tiver `unzip`: `pkg install unzip`. Se a pasta do zip não aparecer
   em `~/storage/downloads`, rode `termux-setup-storage` primeiro e aceite a
   permissão.
4. Quando o `git push` pedir usuário/senha: usuário é seu usuário do GitHub,
   senha é o **token** que você copiou no passo 2 (não é a senha da conta).
5. No navegador, abre o repositório no GitHub → aba **Actions** → vai ter um
   build rodando (ou já terminado) → clica nele → na seção **Artifacts**
   baixa o `drawabattle-gba.zip` → dentro tem o `drawabattle.gba` pronto.
6. Transfere esse `.gba` pro Eony e abre.

Depois disso, qualquer alteração no código: edita os arquivos (no próprio
Termux com `nano`/`vim`, ou em outro editor), `git add . && git commit -m "..."
&& git push`, espera o Actions rodar de novo (1-2 min) e baixa o artifact
atualizado.

## 3. Testar

- Emulador recomendado: **mGBA** (https://mgba.io) — abra o `.gba` direto.
- Hardware real: copie o `.gba` para um flashcart (ex: EZ-Flash).

## Controles

- **D-pad**: mover o personagem pela arena.
- **Segurar A + mover o D-pad = "desenhar" o símbolo da magia**; **soltar A**
  lança o que foi desenhado:
  - soltar sem apertar direção nenhuma → **Estrela** (explosão no seu lugar)
  - segurar **1 direção** só → **Corte** (linha reta na direção que você olha)
  - **ida e volta** (ex: cima → baixo) → **Raio em Cadeia** (salta entre
    inimigos próximos)
  - **3 ou mais direções** (dar a volta) → **Nova Gélida** (círculo que
    atordoa inimigos na área)
- **START**: começa o jogo / reinicia após perder.

O personagem fica parado enquanto você "desenha" (A segurado) — isso é
proposital, pra diferenciar mover de conjurar com só um D-pad.

## Estrutura

```
source/
  gba_regs.h  - registradores de hardware (vídeo, teclado, DMA)
  gfx.c/.h    - desenho (pixel, linha, círculo, retângulo, números)
  input.c/.h  - leitura de botões + reconhecedor de gesto do D-pad
  game.c/.h   - jogador, inimigos, ondas, as 4 magias, HUD
  main.c      - loop principal (sincronizado ao vsync)
Makefile      - build com devkitARM
```

## O que dá pra evoluir a partir daqui

- Trocar o modo bitmap (Mode 3) por **sprites (OBJ) + tiles**: roda muito
  mais rápido no hardware real e permite sprites desenhados à mão em vez de
  círculos/retângulos sólidos.
- Som: devkitARM/libgba dá acesso aos 4 canais de áudio do GBA (2 PSG + 2
  Direct Sound) — dá pra portar os efeitos do jogo original.
- Save: salvar recorde de onda/pontuação em SRAM do cartucho.
- Mais magias/gestos, inimigos com padrões diferentes, chefes de onda.

Se quiser, eu continuo daqui — é só falar o que prioriza (sprites de verdade,
som, mais inimigos, PvP local por link cable, etc.) que eu já parto pro
próximo pedaço.
