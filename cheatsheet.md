# Tips et astuces en C


| Fonction       | Initialisation                                      | Allocation multiple         | Performance             |
|---------------|-------------------------------------------------|----------------------------|-------------------------|
| `malloc(size)` | Alloue `size` octets mais ne les initialise pas (valeurs indéterminées) | Non, alloue un seul bloc | Plus rapide             |
| `calloc(n, size)` | Alloue et initialise à `0` (`n * size` octets) | Oui, alloue `n` blocs de `size` octets | Légèrement plus lent    |
