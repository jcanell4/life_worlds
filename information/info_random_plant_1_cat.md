([Leer en español](info_random_plant_1_spa.md))
([Read in English](info_random_plant_1_eng.md))

# Simulació `random_plant_1`: plantes i agents que aprenen

Configuració: [`config/random_plant_1.json`](../config/random_plant_1.json)

```bash
./build/linux/life_worlds --config config/random_plant_1.json    # Linux
./build/darwin/life_worlds --config config/random_plant_1.json   # macOS
```

## El món

La simulació representa un món (una graella de 24×24) on hi ha dos tipus de plantes i diversos agents del mateix tipus que aprenen per reforç amb una metodologia anomenada QL (Q-learning).

## Les plantes

Una de les plantes, la que està representada com un quadrat, és indigesta i treu energia als agents que se la mengin. L’altra (cercle) és adequada per a l’alimentació dels agents i els aporta energia si se la mengen.

Les plantes viuen durant un temps i passen per diferents etapes. En algunes d’aquestes etapes es poden reproduir i generar noves plantes. Les noves plantes apareixen aleatòriament en alguna cel·la buida.

Per a cada un dels dos tipus de plantes i per a cada una de les seves etapes hi ha una certa taxa de defunció. D’aquesta manera les plantes simulen un cicle de vida en què neixen, creixen, es moren i, si poden, abans de morir es reprodueixen.

La planta quadrada té un cert avantatge reproductiu sobre la rodona. Si les deixéssim sense límits, la primera acabaria dominant tot el món.

Per configuració podem definir que hi hagi un mínim de cada tipus de planta i també podem limitar-ne el nombre màxim, per controlar l’entrenament dels agents i l’entorn on han de viure.

## Els agents

Els agents (triangle) es poden moure pel món en 4 direccions (N/S/E/O) a ritme d’una cel·la per cicle. A cada cicle, l’agent perd una mica de la seva energia. Quan arriba a zero d’energia, es mor.

Els agents no poden traspassar les plantes, però sí que poden decidir tallar-les o bé menjar-se-les. Si un agent decideix tallar una planta, no rep energia. Si decideix menjar-se una planta quadrada, perd una part important de la seva energia. Per contra, si es menja una planta circular, en guanya una part important.

Els agents observen el món amb uns sensors especials que els informen del que hi ha en cada una de les 4 direccions i de si es troba a tocar, a prop, a distància mitjana o lluny. També saben reconèixer si es troben en un nivell d’energia òptim, mig òptim, mig deficient o deficient.

## Plantes menjades i tallades

Les plantes tallades o menjades desapareixen als ulls dels agents, però l’aplicació les mostra perquè l’usuari pugui saber quina ha estat l’acció escollida per l’agent. Així, les plantes menjades les veurem de color gris amb una E (*eaten*) al mig, i les tallades, de color marró amb una K (*killed*) central. Es tracta només d’informació per a l’usuari, que els agents no perceben: a tots els efectes, les cel·les amb aquesta informació són tractades com a cel·les lliures.

## L’aprenentatge

Inicialment, els agents són ximples i fan accions aleatòriament, però a mesura que van aprenent descobreixen que és millor tallar les plantes quadrades i menjar-se les rodones. També descobreixen que tallant una planta quadrada poden accedir a una de rodona, i que cal moure’s per trobar menjar i no morir-se, sobretot si l’energia es troba en nivells no òptims.

Per aconseguir l’aprenentatge, l’agent s’infligeix un seguit de recompenses i càstigs en funció de l’energia rebuda o gastada i en funció del nivell d’energia en què es troba.

## Configuració i entrenament

En el JSON de configuració (`config/random_plant_1.json`) es poden canviar alguns paràmetres de la simulació, tant de plantes com d’agents. En els agents pots indicar si vols que s’entrenin (`train`) o que s’avaluïn (`eval`) amb l’atribut `mode`, i també altres paràmetres referents a l’aprenentatge, la quantitat d’energia rebuda o gastada, etc.

Els agents acumulen la seva experiència en una taula que es guarda com a fitxer binari anomenat `ql_qtable.bin`. Inicialment la taula no existeix i els agents actuen aleatòriament. Canvia l’atribut `mode` a `"train"` a l’apartat de la definició dels agents (`ql_agent`) de manera que es puguin entrenar. En mode entrenament, cada cop que es mor un agent en neix un de nou. Com que la taula de coneixement és compartida, els nous hereten tot el coneixement dels morts.

Executa l’aplicació durant tota una nit i veuràs que els agents han après molt. La taula es desa quan es tanca l’aplicació (tancant la finestra o amb Esc), no abans. Un cop en sàpiguen prou, pots posar-los en mode `"eval"`. En aquest mode, un cop morts no tornen a néixer.

Es pot accelerar l’entrenament amb les tecles F8 / Ctrl+P (que deixen de dibuixar el món) i fent servir múltiples agents, ja que en aquesta simulació tots ells comparteixen la taula.
