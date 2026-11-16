// =============================================================
// WattControl - Gestor inteligente de energia para una lavanderia
// EA2 - Programacion Orientada a Objetos
// =============================================================
#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <ctime>

using namespace std;

// ---------------- Directivas y constantes del negocio ----------------
#define HORAS_DIA 24
#define MAX_CARGAS 3
#define SEMILLA 1

const float POTENCIA_ILUMINACION = 0.8f;
const float POTENCIA_SECADORA = 6.0f;
const float POTENCIA_LAVADORA = 4.0f;
const float POTENCIA_CONTRATADA = 9.0f;
const float PRECIO_CARO = 700.0f;
const float PRECIO_EMERGENCIA = 1500.0f;
const float NIVEL_ALERTA_EMBALSES = 40.0f;
const int PICO_INICIO = 18;
const int PICO_FIN = 20;
const int FALLA_CADA = 20;
const int HORAS_MINIMAS_SECADORA = 4;
const int HORAS_MINIMAS_LAVADORA = 6;
const float NIVEL_EMBALSES_REFERENCIA = 78.0f;

// ---------------- Clase base abstracta ----------------
class Carga {
protected:
    string nombre;
    float potenciaKW;
    bool encendida;
    int horasUso;
    float energiaKWh;

public:
    Carga(string nombreCarga, float potencia)
        : nombre(nombreCarga), potenciaKW(potencia), encendida(false),
          horasUso(0), energiaKWh(0.0f) {}

    virtual ~Carga() {}

    virtual bool debeEncender(int hora, float precio, float nivelEmbalses) = 0;

    void encender() { encendida = true; }
    void apagar() { encendida = false; }

    void registrarHora() {
        if (encendida) {
            horasUso++;
            energiaKWh += potenciaKW;
        }
    }

    float costoHora(float precio) const {
        return encendida ? potenciaKW * precio : 0.0f;
    }

    void reiniciarEstadisticas() {
        horasUso = 0;
        energiaKWh = 0.0f;
        encendida = false;
    }

    string getNombre() const { return nombre; }
    float getPotencia() const { return potenciaKW; }
    bool estaEncendida() const { return encendida; }
    int getHorasUso() const { return horasUso; }
    float getEnergiaKWh() const { return energiaKWh; }
};

// ---------------- Herencia: carga critica ----------------
class CargaCritica : public Carga {
public:
    CargaCritica(string nombreCarga, float potencia)
        : Carga(nombreCarga, potencia) {}

    bool debeEncender(int hora, float precio, float nivelEmbalses) override {
        (void)hora;
        (void)precio;
        (void)nivelEmbalses;
        return true; // Regla 1: la carga critica nunca se apaga.
    }
};

// ---------------- Herencia: cargas aplazables ----------------
class CargaAplazable : public Carga {
private:
    int horasMinimas;

public:
    CargaAplazable(string nombreCarga, float potencia, int minimoHoras)
        : Carga(nombreCarga, potencia), horasMinimas(minimoHoras) {}

    bool debeEncender(int hora, float precio, float nivelEmbalses) override {
        bool embalsesEnAlerta = nivelEmbalses < NIVEL_ALERTA_EMBALSES;
        bool horaPico = hora >= PICO_INICIO && hora <= PICO_FIN;
        bool energiaCara = precio > PRECIO_CARO;

        // Se enciende solamente cuando no hay alerta, hora pico ni energia cara.
        return !(embalsesEnAlerta || horaPico || energiaCara);
    }

    int getHorasMinimas() const { return horasMinimas; }
};

// ---------------- Composicion: medidor pertenece al negocio ----------------
class Medidor {
private:
    float energiaTotalKWh;
    float costoTotal;

public:
    Medidor() : energiaTotalKWh(0.0f), costoTotal(0.0f) {}

    void registrar(float potenciaEncendida, float precio) {
        energiaTotalKWh += potenciaEncendida;
        costoTotal += potenciaEncendida * precio;
    }

    void reiniciar() {
        energiaTotalKWh = 0.0f;
        costoTotal = 0.0f;
    }

    float getEnergiaTotal() const { return energiaTotalKWh; }
    float getCostoTotal() const { return costoTotal; }
};

// ---------------- Asociacion: mercado entrega precio y embalses ----------------
class Mercado {
public:
    float precioHora(int hora) const {
        int minimo = 0;
        int maximo = 0;

        if (hora <= 6) {
            minimo = 398; maximo = 640;
        } else if (hora <= 14) {
            minimo = 456; maximo = 970;
        } else if (hora <= 17) {
            minimo = 596; maximo = 1040;
        } else if (hora <= 20) {
            minimo = 705; maximo = 1612;
        } else {
            minimo = 565; maximo = 970;
        }

        return static_cast<float>(minimo + rand() % (maximo - minimo + 1));
    }

    // Para la simulacion se usa una lectura de embalses indicada por el usuario.
    // El 78 % es la referencia nacional reportada para septiembre de 2026.
    float nivelEmbalses(float lecturaEscenario) const {
        return lecturaEscenario;
    }
};

// ---------------- Agregacion: las cargas se crean fuera del negocio ----------------
class Negocio {
private:
    string nombre;
    Carga* cargas[MAX_CARGAS];
    int cantidadCargas;
    Medidor medidor;       // Composicion: el medidor es parte del negocio.
    Mercado* mercado;      // Asociacion: el negocio conoce al mercado.

    float costoSinGestor;
    int horasSimuladas;
    int fallasLectura;
    bool huboSimulacion;
    bool emergenciaRacionamiento;
    bool alertaPotencia;
    float nivelEmbalsesUsado;

    float potenciaEncendida() const {
        float total = 0.0f;
        for (int i = 0; i < cantidadCargas; i++) {
            if (cargas[i]->estaEncendida()) total += cargas[i]->getPotencia();
        }
        return total;
    }

    void aplicarLimitePotencia() {
        if (potenciaEncendida() > POTENCIA_CONTRATADA) {
            int indiceMayor = -1;
            float mayorPotencia = -1.0f;

            // Busca la carga aplazable de mayor potencia.
            for (int i = 0; i < cantidadCargas; i++) {
                CargaAplazable* aplazable = dynamic_cast<CargaAplazable*>(cargas[i]);
                if (aplazable != nullptr && cargas[i]->estaEncendida() &&
                    cargas[i]->getPotencia() > mayorPotencia) {
                    mayorPotencia = cargas[i]->getPotencia();
                    indiceMayor = i;
                }
            }

            if (indiceMayor != -1) {
                cargas[indiceMayor]->apagar();
                alertaPotencia = true;
                cout << "  ALERTA: se apago " << cargas[indiceMayor]->getNombre()
                     << " por superar la potencia contratada.\n";
            }
        }
    }

public:
    Negocio(string nombreNegocio, Mercado* mercadoExterno)
        : nombre(nombreNegocio), cantidadCargas(0), mercado(mercadoExterno),
          costoSinGestor(0.0f), horasSimuladas(0), fallasLectura(0),
          huboSimulacion(false), emergenciaRacionamiento(false),
          alertaPotencia(false), nivelEmbalsesUsado(NIVEL_EMBALSES_REFERENCIA) {
        for (int i = 0; i < MAX_CARGAS; i++) cargas[i] = nullptr;
    }

    void agregarCarga(Carga* carga) {
        if (cantidadCargas < MAX_CARGAS) {
            cargas[cantidadCargas] = carga;
            cantidadCargas++;
        } else {
            cout << "No hay espacio para agregar otra carga.\n";
        }
    }

    void mostrarCargas() const {
        cout << "\nNEGOCIO: " << nombre << "\n";
        cout << "Potencia contratada: " << POTENCIA_CONTRATADA << " kW\n";
        cout << left << setw(25) << "Carga" << setw(15) << "Tipo" << "Potencia\n";
        cout << string(52, '-') << "\n";
        for (int i = 0; i < cantidadCargas; i++) {
            string tipo = (dynamic_cast<CargaCritica*>(cargas[i]) != nullptr)
                              ? "Critica" : "Aplazable";
            cout << left << setw(25) << cargas[i]->getNombre()
                 << setw(15) << tipo << cargas[i]->getPotencia() << " kW\n";
        }
        cout << right;
    }

    void evaluar(int hora, float precio, float nivel) {
        cout << "\nEvaluacion para las " << hora << ":00 | Precio: $"
             << fixed << setprecision(0) << precio
             << " COP/kWh | Embalses: " << nivel << "%\n";

        // Regla 2: informar claramente el riesgo cuando los embalses bajan del 40 %.
        if (nivel < NIVEL_ALERTA_EMBALSES) {
            cout << "ALERTA: Riesgo de racionamiento. Embalses por debajo del 40 %.\n";
            cout << "Solo funciona la carga critica.\n";
        }

        for (int i = 0; i < cantidadCargas; i++) {
            bool decision = cargas[i]->debeEncender(hora, precio, nivel);
            cargas[i]->encender();
            if (!decision) cargas[i]->apagar();
        }
        aplicarLimitePotencia();

        for (int i = 0; i < cantidadCargas; i++) {
            cout << "  " << left << setw(25) << cargas[i]->getNombre()
                 << (cargas[i]->estaEncendida() ? "ENCENDER" : "APLAZAR/APAGAR") << "\n";
        }
        cout << right;
    }

    void simularDia(float nivelInicialEmbalses) {
        medidor.reiniciar();
        costoSinGestor = 0.0f;
        horasSimuladas = 0;
        fallasLectura = 0;
        emergenciaRacionamiento = false;
        alertaPotencia = false;
        huboSimulacion = true;
        nivelEmbalsesUsado = mercado->nivelEmbalses(nivelInicialEmbalses);

        for (int i = 0; i < cantidadCargas; i++) cargas[i]->reiniciarEstadisticas();

        cout << "\nSIMULACION DE 24 HORAS - " << nombre << "\n";
        cout << "Nivel de embalses del escenario: " << nivelEmbalsesUsado << "%\n";
        if (nivelEmbalsesUsado < NIVEL_ALERTA_EMBALSES) {
            cout << "ALERTA: Riesgo de racionamiento. Embalses por debajo del 40 %.\n";
            cout << "Solo funciona la carga critica durante el escenario.\n";
        }
        cout << left << setw(6) << "Hora" << setw(10) << "Precio"
             << setw(12) << "Embalses" << setw(14) << "Ilum./caja"
             << setw(14) << "Secadora" << setw(14) << "Lavadora"
             << "Costo hora\n";
        cout << string(91, '-') << "\n";

        for (int hora = 0; hora < HORAS_DIA; hora++) {
            float precio = mercado->precioHora(hora);

            // Regla break: emergencia si el precio supera el limite y los embalses < 40 %.
            if (precio > PRECIO_EMERGENCIA &&
                nivelEmbalsesUsado < NIVEL_ALERTA_EMBALSES) {
                // Incluso durante la emergencia, la carga critica permanece encendida.
                for (int i = 0; i < cantidadCargas; i++) {
                    if (dynamic_cast<CargaCritica*>(cargas[i]) != nullptr) {
                        cargas[i]->encender();
                    } else {
                        cargas[i]->apagar();
                    }
                }

                horasSimuladas++;
                float potenciaHora = potenciaEncendida();
                for (int i = 0; i < cantidadCargas; i++) {
                    cargas[i]->registrarHora();
                }
                medidor.registrar(potenciaHora, precio);

                // La comparacion sin gestor contempla las tres cargas en esta hora.
                float potenciaSinGestor = POTENCIA_ILUMINACION +
                                          POTENCIA_SECADORA + POTENCIA_LAVADORA;
                costoSinGestor += potenciaSinGestor * precio;

                cout << setw(6) << hora << setw(10) << precio << setw(12)
                     << nivelEmbalsesUsado << setw(14) << "ON"
                     << setw(14) << "OFF" << setw(14) << "OFF"
                     << "$" << fixed << setprecision(0) << potenciaHora * precio
                     << "\nEMERGENCIA DE RACIONAMIENTO: termina la simulacion.\n";
                emergenciaRacionamiento = true;
                break;
            }

            horasSimuladas++;
            // Comparacion: sin gestor las tres cargas funcionarian durante esta hora.
            float potenciaSinGestor = POTENCIA_ILUMINACION + POTENCIA_SECADORA + POTENCIA_LAVADORA;
            costoSinGestor += potenciaSinGestor * precio;

            // Regla continue: falla una lectura exactamente cada 20 horas procesadas.
            // En una jornada de 24 horas ocurre en la hora 19 (vigésima hora).
            bool falla = ((hora + 1) % FALLA_CADA == 0);
            if (falla) {
                fallasLectura++;
                for (int i = 0; i < cantidadCargas; i++) {
                    if (dynamic_cast<CargaCritica*>(cargas[i]) != nullptr) {
                        cargas[i]->encender();
                    } else {
                        cargas[i]->apagar();
                    }
                }
                float potenciaHora = potenciaEncendida();
                for (int i = 0; i < cantidadCargas; i++) {
                    cargas[i]->registrarHora();
                }
                medidor.registrar(potenciaHora, precio);
                cout << setw(6) << hora << setw(10) << precio << setw(12)
                     << nivelEmbalsesUsado << setw(14) << "ON"
                     << setw(14) << "FALLA" << setw(14) << "FALLA"
                     << "$" << fixed << setprecision(0) << potenciaHora * precio
                     << " (falla: solo critica)\n";
                continue;
            }

            // Decision polimorfica: el mismo ciclo consulta cada clase hija.
            for (int i = 0; i < cantidadCargas; i++) {
                if (cargas[i]->debeEncender(hora, precio, nivelEmbalsesUsado)) {
                    cargas[i]->encender();
                } else {
                    cargas[i]->apagar();
                }
            }

            // Regla 6: si se supera la potencia contratada, se apaga la aplazable mas potente.
            aplicarLimitePotencia();

            float potenciaHora = potenciaEncendida();
            float costoHora = 0.0f;
            for (int i = 0; i < cantidadCargas; i++) {
                cargas[i]->registrarHora();
                costoHora += cargas[i]->costoHora(precio);
            }
            medidor.registrar(potenciaHora, precio);

            cout << setw(6) << hora << setw(10) << precio << setw(12)
                 << nivelEmbalsesUsado;
            for (int i = 0; i < cantidadCargas; i++) {
                cout << setw(14) << (cargas[i]->estaEncendida() ? "ON" : "OFF");
            }
            cout << "$" << fixed << setprecision(0) << costoHora << "\n";
        }

        cout << "\nSimulacion finalizada. Consulte la opcion 4 para ver el resumen.\n";
    }

    void resumen() const {
        if (!huboSimulacion) {
            cout << "Todavia no se ha realizado una simulacion.\n";
            return;
        }

        cout << "\n========== RESUMEN DE WATTCONTROL ==========\n";
        cout << "Negocio: " << nombre << "\n";
        cout << "Horas procesadas: " << horasSimuladas << " de " << HORAS_DIA << "\n";
        cout << "Fallas de lectura: " << fallasLectura << "\n";
        cout << "Nivel de embalses usado: " << nivelEmbalsesUsado << "%\n\n";

        bool produccionIncompleta = false;
        for (int i = 0; i < cantidadCargas; i++) {
            cout << left << setw(25) << cargas[i]->getNombre()
                 << setw(5) << cargas[i]->getHorasUso() << " horas | "
                 << fixed << setprecision(1) << cargas[i]->getEnergiaKWh() << " kWh";

            CargaAplazable* aplazable = dynamic_cast<CargaAplazable*>(cargas[i]);
            if (aplazable != nullptr) {
                cout << " | minimo: " << aplazable->getHorasMinimas() << " h";
                if (cargas[i]->getHorasUso() < aplazable->getHorasMinimas()) {
                    produccionIncompleta = true;
                    cout << " | NO CUMPLIDO";
                } else {
                    cout << " | CUMPLIDO";
                }
            }
            cout << "\n";
        }

        float costoCon = medidor.getCostoTotal();
        float ahorro = costoSinGestor - costoCon;
        float porcentajeAhorro = (costoSinGestor > 0.0f)
                                     ? (ahorro / costoSinGestor) * 100.0f : 0.0f;

        cout << right << "\nEnergia con gestor: " << medidor.getEnergiaTotal() << " kWh\n";
        cout << "Costo con gestor:   $" << fixed << setprecision(0) << costoCon << " COP\n";
        cout << "Costo sin gestor:   $" << costoSinGestor << " COP\n";
        cout << "Ahorro:             $" << ahorro << " COP ("
             << setprecision(1) << porcentajeAhorro << " %)\n";

        if (produccionIncompleta) cout << "ALERTA: produccion incompleta.\n";
        else cout << "Produccion minima cumplida para las cargas aplazables.\n";
        if (emergenciaRacionamiento) cout << "La simulacion termino por emergencia de racionamiento.\n";
        if (alertaPotencia) cout << "Se activo la proteccion por potencia contratada.\n";
        cout << "============================================\n";
    }
};

// ---------------- Validaciones de entrada ----------------
int leerEnteroRango(const string& mensaje, int minimo, int maximo) {
    int valor;
    do {
        cout << mensaje;
        cin >> valor;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "Entrada invalida. Ingrese un numero entero.\n";
            valor = minimo - 1;
        } else if (valor < minimo || valor > maximo) {
            cout << "Valor fuera del rango permitido.\n";
        }
    } while (valor < minimo || valor > maximo);
    return valor;
}

float leerFloatMinimo(const string& mensaje, float minimo) {
    float valor;
    do {
        cout << mensaje;
        cin >> valor;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "Entrada invalida. Ingrese un numero.\n";
            valor = minimo - 1.0f;
        } else if (valor <= minimo) {
            cout << "El valor debe ser mayor que " << minimo << ".\n";
        }
    } while (valor <= minimo);
    return valor;
}

// ---------------- Programa principal ----------------
int main() {
    srand(SEMILLA);

    Mercado mercado;
    CargaCritica iluminacionCaja("Iluminacion y caja", POTENCIA_ILUMINACION);
    CargaAplazable secadora("Secadora", POTENCIA_SECADORA, HORAS_MINIMAS_SECADORA);
    CargaAplazable lavadora("Lavadora industrial", POTENCIA_LAVADORA, HORAS_MINIMAS_LAVADORA);

    Negocio lavanderia("Lavanderia WattControl", &mercado);
    lavanderia.agregarCarga(&iluminacionCaja);
    lavanderia.agregarCarga(&secadora);
    lavanderia.agregarCarga(&lavadora);

    int opcion;
    int horaManual = 12;
    float precioManual = 600.0f;
    float embalsesManual = NIVEL_EMBALSES_REFERENCIA;

    cout << "==============================================\n";
    cout << "       WATTCONTROL - LAVANDERIA\n";
    cout << "==============================================\n";
    lavanderia.mostrarCargas();

    do {
        cout << "\n--------------- MENU PRINCIPAL ---------------\n";
        cout << "1. Ingresar lectura manual (hora, precio, embalses)\n";
        cout << "2. Evaluar decision de cada carga\n";
        cout << "3. Simular un dia completo (24 horas)\n";
        cout << "4. Ver resumen de la ultima simulacion\n";
        cout << "0. Salir\n";
        cout << "Seleccione una opcion: ";
        cin >> opcion;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(10000, '\n');
            opcion = -1;
        }

        switch (opcion) {
            case 1:
                horaManual = leerEnteroRango("Ingrese la hora (0 a 23): ", 0, 23);
                precioManual = leerFloatMinimo("Ingrese el precio COP/kWh (mayor que 0): ", 0.0f);
                embalsesManual = leerEnteroRango("Ingrese nivel de embalses (0 a 100 %): ", 0, 100);
                cout << "Lectura guardada correctamente.\n";
                break;

            case 2:
                lavanderia.evaluar(horaManual, precioManual, embalsesManual);
                break;

            case 3: {
                int nivelInicial = leerEnteroRango(
                    "Nivel de embalses para el escenario del dia (0 a 100 %): ", 0, 100);
                lavanderia.simularDia(static_cast<float>(nivelInicial));
                break;
            }

            case 4:
                lavanderia.resumen();
                break;

            case 0:
                cout << "Gracias por utilizar WattControl. Hasta pronto.\n";
                break;

            default:
                cout << "Opcion no valida. Seleccione una opcion del menu.\n";
        }
    } while (opcion != 0);

    return 0;
}
