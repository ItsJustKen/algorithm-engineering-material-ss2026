Der grund für die mispredictions ist die Zeile 70.
Da t1 zufällig ist, existiert kein Muster.
Die mispredictions können verhindert werden, indem anstatt einer if bedingung t1 und t2 berechnet werden und nutzen eine boolean maske um die ergebnisse zu verbinden
| N | original | impoved |
|----------|----------|----------|
|1024    | 0.00686036 seconds  | 0.0119339 seconds   |
| 8192    | 0.379373 seconds   | 2.42895 seconds   |
| 32768   | 6.04442 seconds   | 80.1473 seconds   |
