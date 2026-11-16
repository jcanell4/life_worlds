([Llegeix en català](info_energy_storage_plant_1_cat.md))
([Read in English](info_energy_storage_plant_1_eng.md))

# Simulación `energy_storage_plant_1`: plantas que almacenan energía

Configuración: [`config/energy_storage_plant_1.json`](../config/energy_storage_plant_1.json)

```bash
./build/linux/life_worlds --config config/energy_storage_plant_1.json    # Linux
./build/darwin/life_worlds --config config/energy_storage_plant_1.json   # macOS
```

## El mundo

La simulación representa un mundo (una cuadrícula de 24×24) donde solo hay plantas: dos tipos de plantas que acumulan energía y que, cuando tienen suficiente, se dividen y ocupan una celda vecina. No hay agentes.

## Las plantas

Uno de los tipos (`moss`) se representa como un hexágono; el otro (`vine`), como un rombo.

Al principio aparecen, en celdas vacías elegidas al azar, 3 plantas `moss` y 2 plantas `vine`. Cada una de estas plantas iniciales es la fundadora de una filiación: todas las plantas que desciendan de ella pertenecerán a la misma filiación.

En cada ciclo, cada planta gana un poco de energía, hasta llegar a su máximo. La `moss` crece más deprisa (5 unidades por ciclo, hasta 100) que la `vine` (3 unidades por ciclo, hasta 80).

Cuando una planta llega al máximo de energía, se divide: crea una planta nueva del mismo tipo en una de las celdas contiguas libres (arriba, abajo, izquierda o derecha), elegida al azar. La planta nueva recibe una parte de la energía de la madre y la madre se queda el resto. La `moss` reparte la energía a partes iguales; la `vine` da el 40 % y se queda el 60 %. Después, madre e hija vuelven a acumular energía hasta que se pueden dividir de nuevo.

Una planta que no tiene ninguna celda contigua libre queda detenida: ni acumula energía ni se divide.

Las plantas no mueren. Lo que limita el crecimiento es el tamaño de la filiación: una filiación de `moss` puede ocupar como máximo 12 celdas, y una de `vine`, 20. Cuando una filiación llega a su límite, sus plantas terminan de llenarse de energía y ya no se dividen más.

## Lo que se ve en pantalla

El brillo de cada planta indica la energía que tiene almacenada: oscura cuando tiene poca y brillante cuando está llena. Así se ve cómo cada planta se va cargando y cómo, al dividirse, madre e hija se oscurecen de golpe.

Todas las `moss` comparten el mismo color (verde azulado), definido en la configuración. La `vine` no tiene ningún color definido, y por eso cada filiación de `vine` se dibuja con un color propio: así se puede ver hasta dónde se ha extendido cada una.

La barra inferior muestra el ciclo actual y cuántas plantas vivas hay de cada tipo.

## Cómo evoluciona

Durante los primeros ciclos parece que no pasa nada: las plantas solo se llenan. La primera división de las `moss` llega en el ciclo 19, y la de las `vine`, en el ciclo 26. A partir de ahí las manchas crecen cada vez más deprisa, porque cada planta nueva también se divide. Las `moss` completan antes sus filiaciones; las `vine` tardan más, pero acaban ocupando más espacio.

Como hay 3 filiaciones de `moss` y 2 de `vine`, el mundo acaba teniendo como máximo 36 `moss` (3 × 12) y 40 `vine` (2 × 20): 76 plantas en una cuadrícula de 576 celdas. Con la semilla de la configuración (`rng_seed` 42) se llega a ese punto hacia el ciclo 100, unos 10 segundos a la velocidad configurada (10 ciclos por segundo).

Poco después el mundo queda estable y ya no cambia: las plantas que tienen alguna celda libre al lado acaban llenas, y las que han quedado rodeadas se quedan con la energía que tenían.

## Configuración

En el JSON de configuración (`config/energy_storage_plant_1.json`) se pueden cambiar los parámetros de cada tipo de planta: cuántas plantas iniciales hay (`initial_count`) y con cuánta energía empiezan (`initial_energy`), la energía que ganan en cada ciclo (`growing_rate`), la energía máxima (`maximum_energy_limit`), el número máximo de celdas por filiación (`maximum_cell_occupancy`), la parte de la energía que recibe la planta nueva (`split_fraction`) y el aspecto (`visual`).

Por ejemplo, si aumentas mucho `maximum_cell_occupancy`, las filiaciones acabarán chocando unas con otras y disputándose el espacio. Si cambias `rng_seed`, las plantas iniciales aparecerán en otros sitios.

## Limitaciones

De momento estas plantas no interactúan con el resto de elementos: los agentes QL y las plantas de la simulación `random_plant_1` las ignoran, y por eso todavía no se combinan en una misma configuración.
