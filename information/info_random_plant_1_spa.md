([Llegeix en català](info_random_plant_1_cat.md))
([Read in English](info_random_plant_1_eng.md))

# Simulación `random_plant_1`: plantas y agentes que aprenden

Configuración: [`config/random_plant_1.json`](../config/random_plant_1.json)

```bash
./build/linux/life_worlds --config config/random_plant_1.json    # Linux
./build/darwin/life_worlds --config config/random_plant_1.json   # macOS
```

## El mundo

La simulación representa un mundo (una cuadrícula de 24×24) donde hay dos tipos de plantas y varios agentes del mismo tipo que aprenden por refuerzo con una metodología llamada QL (Q-learning).

## Las plantas

Una de las plantas, la que está representada como un cuadrado, es indigesta y quita energía a los agentes que se la coman. La otra (círculo) es adecuada para la alimentación de los agentes y les aporta energía si se la comen.

Las plantas viven durante un tiempo y pasan por diferentes etapas. En algunas de estas etapas se pueden reproducir y generar nuevas plantas. Las nuevas plantas aparecen aleatoriamente en alguna celda vacía.

Para cada uno de los dos tipos de plantas y para cada una de sus etapas hay una cierta tasa de defunción. De este modo las plantas simulan un ciclo de vida en el que nacen, crecen, mueren y, si pueden, antes de morir se reproducen.

La planta cuadrada tiene cierta ventaja reproductiva sobre la redonda. Si las dejáramos sin límites, la primera acabaría dominando todo el mundo.

Por configuración podemos definir que haya un mínimo de cada tipo de planta y también podemos limitar su número máximo, para controlar el entrenamiento de los agentes y el entorno donde tienen que vivir.

## Los agentes

Los agentes (triángulo) se pueden mover por el mundo en 4 direcciones (N/S/E/O) a ritmo de una celda por ciclo. En cada ciclo, el agente pierde un poco de su energía. Cuando llega a cero de energía, muere.

Los agentes no pueden atravesar las plantas, pero sí pueden decidir cortarlas o bien comérselas. Si un agente decide cortar una planta, no recibe energía. Si decide comerse una planta cuadrada, pierde una parte importante de su energía. En cambio, si se come una planta circular, gana una parte importante.

Los agentes observan el mundo con unos sensores especiales que les informan de lo que hay en cada una de las 4 direcciones y de si se encuentra tocando, cerca, a distancia media o lejos. También saben reconocer si se encuentran en un nivel de energía óptimo, medio óptimo, medio deficiente o deficiente.

## Plantas comidas y cortadas

Las plantas cortadas o comidas desaparecen a ojos de los agentes, pero la aplicación las muestra para que el usuario pueda saber cuál ha sido la acción escogida por el agente. Así, las plantas comidas las veremos de color gris con una E (*eaten*) en el centro, y las cortadas, de color marrón con una K (*killed*) central. Se trata solo de información para el usuario, que los agentes no perciben: a todos los efectos, las celdas con esta información se tratan como celdas libres.

## El aprendizaje

Inicialmente, los agentes son tontos y hacen acciones aleatoriamente, pero a medida que van aprendiendo descubren que es mejor cortar las plantas cuadradas y comerse las redondas. También descubren que cortando una planta cuadrada pueden acceder a una redonda, y que hay que moverse para encontrar comida y no morirse, sobre todo si la energía se encuentra en niveles no óptimos.

Para conseguir el aprendizaje, el agente se inflige una serie de recompensas y castigos en función de la energía recibida o gastada y en función del nivel de energía en el que se encuentra.

## Configuración y entrenamiento

En el JSON de configuración (`config/random_plant_1.json`) se pueden cambiar algunos parámetros de la simulación, tanto de plantas como de agentes. En los agentes puedes indicar si quieres que se entrenen (`train`) o que se evalúen (`eval`) con el atributo `mode`, y también otros parámetros referentes al aprendizaje, la cantidad de energía recibida o gastada, etc.

Los agentes acumulan su experiencia en una tabla que se guarda como fichero binario llamado `ql_qtable.bin`. Inicialmente la tabla no existe y los agentes actúan aleatoriamente. Cambia el atributo `mode` a `"train"` en el apartado de la definición de los agentes (`ql_agent`) para que se puedan entrenar. En modo entrenamiento, cada vez que muere un agente nace uno nuevo. Como la tabla de conocimiento es compartida, los nuevos heredan todo el conocimiento de los muertos.

Ejecuta la aplicación durante toda una noche y verás que los agentes han aprendido mucho. La tabla se guarda cuando se cierra la aplicación (cerrando la ventana o con Esc), no antes. Una vez sepan lo suficiente, puedes ponerlos en modo `"eval"`. En este modo, una vez muertos no vuelven a nacer.

Se puede acelerar el entrenamiento con las teclas F8 / Ctrl+P (que dejan de dibujar el mundo) y usando múltiples agentes, ya que en esta simulación todos ellos comparten la tabla.
