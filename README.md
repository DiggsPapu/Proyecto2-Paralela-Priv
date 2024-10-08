# Proyecto2-Paralela-Priv

## Members

* Brian Carrillo
* Carlos López
* Diego Alonzo 

## Container

To run the project using a container, follow these steps:

1. Clone the repository
   ```
   git clone https://github.com/DiggsPapu/Proyecto2-Paralela-Priv.git
   ```
4. Build image
   ```
   docker build -t proyecto02 .
   ```
5. Run image
   ```
   docker run -it proyecto02
   ```

## Executing the programs 

To run the sequential algorithm:
```
make -f MakefileSecuencial run
```
To run the naive algorithm:
```
make -f MakefileAlternative run
```
To run the OpenMP algorithm:
```
make -f MakefileAlternative2 run
```
To run the Divisions algorithm:
```
make -f MakefileAlternative3 run
```
