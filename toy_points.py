import numpy as np

# Graine aléatoire fixe : on obtient les MÊMES points à chaque exécution
rng = np.random.default_rng(seed=42)

# 500 points, chacun avec 3 coordonnées (x, y, z), dans un cube de côté L
L = 1000.0
points = rng.uniform(0.0, L, size=(500, 3))

# Écrit un fichier texte de 3 colonnes (x y z) = le format d'entrée de DIVE
np.savetxt("toy.txt", points, fmt="%.6f")

print(f"{len(points)} points écrits dans toy.txt")