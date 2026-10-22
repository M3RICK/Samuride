# Où on en est, quoi faire ensuite

*Mis à jour le 22/10/2026.*

## L'état des branches

- **`main`** contient la v2.9, fusionnée par la PR #13. Elle a les snipers Arasaka, les couloirs de
  zappers et le fichier de réglages `assets/tuning.txt`. Le Hunter y est **caché** : son code est là,
  mais il n'est pas jouable.
  - **Pas encore taguée.** La v2.9 n'est pas encore publiée : la dernière release est la v2.85.
- **`feature/hunter`** : le Hunter complet. C'est là qu'on bosse sur lui.

## Pour sortir la v2.9

Taguer `main` en `v2.9` et pousser le tag. La release se construit seule, avec les notes de la
section v2.9 du `CHANGELOG.md`.

## Pour reprendre le Hunter

```
git checkout feature/hunter
git merge main
git revert d5a1f82      # le commit qui le cache sur main : il revient
```

Ce qui reste à faire sur lui est listé dans `docs/IDEAS.md`, section « The Hunter » :
- une ligne de vol dessinée exprès, et une vraie brume ;
- la planche de visages pour le HUD ;
- un titan stun qu'on voit vraiment ;
- revoir ses balles ;
- le différencier de Mikasa ;
- son ultimate.

## Pour que tes potes testent l'équilibrage

Ils ouvrent `assets/tuning.txt` dans le dossier du jeu, changent des chiffres et relancent. Ensuite
ils t'envoient le fichier avec leurs impressions. Tout est expliqué en haut du fichier.

## La suite, dans l'ordre

1. **Les outils pour les potes** (`docs/IDEAS.md`, section « Easy to change ») : les persos dans un
   fichier texte, une page web pour découper les planches, recharger les réglages sans relancer le jeu.
2. **Dante** : il faut d'abord sa planche de sprites (même méthode que pour le Hunter).
3. **Adam Smasher**, le premier boss, avant 1000 m.
