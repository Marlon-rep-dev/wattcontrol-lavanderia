# WattControl - Gestor de energía para una lavandería

## Descripción
WattControl es un programa desarrollado en C++ para gestionar el consumo de energía de una lavandería. Su función es decidir qué equipos pueden funcionar teniendo en cuenta el precio de la energía, las horas pico, el nivel de los embalses y la potencia contratada.

## Equipos del negocio
- Iluminación y caja: 0.8 kW.
- Secadora: 6 kW.
- Lavadora industrial: 4 kW.

## Clases principales
El proyecto utiliza las siguientes clases:
- `Carga`: clase base abstracta.
- `CargaCritica`: representa la carga que siempre debe funcionar.
- `CargaAplazable`: representa los equipos que pueden aplazarse.
- `Medidor`: registra el consumo y los costos.
- `Mercado`: proporciona los precios de energía y el nivel de los embalses.
- `Negocio`: administra las cargas y realiza la simulación.

## Conceptos de programación orientada a objetos
Se aplican herencia, polimorfismo, composición, agregación y asociación.

## Compilación y ejecución
Para compilar el programa utilizando G++:

```bash
g++ wattcontrol.cpp -o wattcontrol.exe
```

Para ejecutarlo en Windows:

```powershell
.\wattcontrol.exe
```

