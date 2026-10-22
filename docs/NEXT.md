# Où on en est, quoi faire ensuite

*Mis à jour le 23/10/2026.*

## L'état des branches

- **`main`** contient la v2.9, fusionnée par la PR #13. Elle a les snipers Arasaka, les couloirs de
  zappers et le fichier de réglages `assets/tuning.txt`.
  - **Pas encore taguée.** La v2.9 n'est pas encore publiée : la dernière release est la v2.85.
- **`2.95`** : le grand ménage du code, fait le 23/10, pas encore poussé ni joué. Le Hunter en est
  retiré ; les armes et les ultimes de chaque runner sont des classes (`src/server/arsenal.cpp`), les
  titans et les snipers aussi (`Horde`, `SniperSquad`) ; les `.o` vont dans `build/obj`. Aucun
  changement de gameplay : à vérifier en jouant chaque runner.
- **`feature/hunter`** : le Hunter, abandonné le 23/10 (aucune planche n'était assez belle). On la
  garde pour son ultime, la cloche du Old Hunter, à reprendre pour le Doppelganger de Dante.

## Pour sortir la v2.9

Taguer `main` en `v2.9` et pousser le tag. La release se construit seule, avec les notes de la
section v2.9 du `CHANGELOG.md`.

## Pour que tes potes testent l'équilibrage

Ils ouvrent `assets/tuning.txt` dans le dossier du jeu, changent des chiffres et relancent. Ensuite
ils t'envoient le fichier avec leurs impressions. Tout est expliqué en haut du fichier.

## La suite, dans l'ordre

1. **Les outils pour les potes** (`docs/IDEAS.md`, section « Easy to change ») : les persos dans un
   fichier texte, une page web pour découper les planches, recharger les réglages sans relancer le jeu.
2. **Dante**, une fois le ménage fini : il faut d'abord sa planche de sprites.
3. **Adam Smasher**, le premier boss, avant 1000 m.
