# Fonts (Fredoka, Nunito)

The launcher's screen (launcher/ui/) uses Fredoka for headings and buttons and Nunito for body text. Both are under the
SIL Open Font License 1.1: each folder keeps its OFL.txt, whose first line names the copyright holder. The fonts are
compiled into the launcher as byte arrays (CMakeLists.txt writes them into the build folder at configure time). The OFL
allows that, as long as the license travels with the program and the fonts are not sold on their own. The game's own
fonts are never used here.

stb_truetype draws only a variable font's default instance, which is a light weight for both families. Static TTFs of
the weights we need are therefore taken from each project's own repository, the one Google Fonts names as its source.
Neither project publishes GitHub releases or checksums. The SHA-256 values below are of our download. Each file's git
blob id was checked against the one GitHub's tree listing gives for that commit, and all match.

Downloaded 2026-10-22 from https://raw.githubusercontent.com/<repository>/<commit>/<path>.

## fredoka/

- Project: Fredoka. https://github.com/hafontia-zz/Fredoka-One, which the OFL text
  gives under its earlier address, https://github.com/hafontia/Fredoka-One.
- Commit 35c584ff23450c9bcdf8819706e12fcdeefe1712 (gh-pages, 2022-03-03). It is the commit Google Fonts'
  ofl/fredoka/METADATA.pb names.
- Version 2.001 (head.fontRevision and name ID 5), static TTFs from fonts/ttf/.
- License: SIL Open Font License 1.1 (OFL.txt, copyright 2016 The Fredoka Project Authors). No Reserved Font Name.

| File | Bytes | SHA-256 | git blob |
|---|---|---|---|
| Fredoka-Bold.ttf | 47128 | 457525f9e9296784b79e3bb12b808f739ee802404f51043ae5094ce783a6e52c | 0a729a388457e47c6946e5e0986a2fe6388c1a4d |
| Fredoka-SemiBold.ttf | 47108 | 95839d50cc746b491c4710673be1d2cc8179a132f9600851810863922ddc12f9 | 309c649f1d644e9e936dcb065e964f55aa1eab18 |
| OFL.txt | 4388 | 5c9e7eee5c6b25f4b05b8d53b2e470ea4962f9ced742d044a98f7d95d1375bab | 791a98de7893b9bb62d06729745d00655e791a40 |

## nunito/

- Project: Nunito. https://github.com/googlefonts/nunito.
- Commit ed889dda6d742a0495c1963898e1b3567ab61a15 (main, 2020-11-24), the last commit that holds static TTFs in
  fonts/TTF/. The project removed them on 2021-10-29 ("Delete obsolete files") and now publishes only variable fonts,
  whose default instance is ExtraLight.
- Version 3.601 (head.fontRevision and name ID 5).
- License: SIL Open Font License 1.1 (OFL.txt, copyright 2014 The Nunito Project Authors). No Reserved Font Name.

| File | Bytes | SHA-256 | git blob |
|---|---|---|---|
| Nunito-Regular.ttf | 154312 | 880ead818af373b0da8c2a3e493f7d0a9c1db149a67a867b04f0952c475d667a | 1f038185e25704b4926508c7381943452bc5d91a |
| Nunito-Bold.ttf | 154140 | f922b74ed5cca7c8675d352fd5ed197e0f38b3ac4a427c15b7a3db039dc6294c | f41ef2036a9d683a52da7d98df6d2e08c2245d02 |
| OFL.txt | 4385 | 580df76c95a1ec5ab878ceb25bb3d85c6a076804e9c970c8c6972aea775fdf65 | c8210f08ca3cece05b4f7baa428afe8e1abe3293 |

No local changes.
