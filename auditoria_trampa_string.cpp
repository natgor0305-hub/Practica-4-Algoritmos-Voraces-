#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>


void compresor_ingenuo_trampa(const std::string& texto_entrada,
                              const std::vector<std::string>& tabla_codigos,
                              const std::string& archivo_salida) {
    std::string gran_cadena_bits = "";
    size_t realocaciones = 0;
    size_t capacidad_previa = gran_cadena_bits.capacity();

    for (unsigned char c : texto_entrada) {
        gran_cadena_bits += tabla_codigos[c]; 
        if (gran_cadena_bits.capacity() != capacidad_previa) {
            realocaciones++;
            capacidad_previa = gran_cadena_bits.capacity();
        }
    }

    std::ofstream out(archivo_salida, std::ios::binary);
    out << gran_cadena_bits; 
    out.close();

    std::cout << "Bytes originales: " << texto_entrada.size() << " bytes\n";
    std::cout << "Bytes guardados por la trampa std::string: " << gran_cadena_bits.size() << " bytes\n";
    std::cout << "Realocaciones dinamicas de std::string: " << realocaciones << "\n";
}

int main() {
    std::vector<std::string> codigos_ejemplo(256, "01011"); 
    std::string muestra(100000, 'A'); 

    std::cout << "=== AUDITORIA: TRAMPA DE CADENAS DE TEXTO BINARIAS ===\n";
    compresor_ingenuo_trampa(muestra, codigos_ejemplo, "salida_trampa.txt");
    std::remove("salida_trampa.txt");
    return 0;
}
