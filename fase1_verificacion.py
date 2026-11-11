import math
import heapq
from collections import Counter


MENSAJE = "ABRACADABRA_ALGORITMO_AVANZADO_ESCOM"

class NodoHuffman:
    def __init__(self, frec, simbolo, izq=None, der=None, orden=0):
        self.frec = frec
        self.simbolo = simbolo
        self.izq = izq
        self.der = der
        self.orden = orden

    def __lt__(self, otro):
        if self.frec != otro.frec:
            return self.frec < otro.frec
        return self.orden < otro.orden

def generar_codigos(nodo, prefijo="", tabla=None):
    if tabla is None:
        tabla = {}
    if nodo.izq is None and nodo.der is None:
        tabla[nodo.simbolo] = prefijo if prefijo else "0"
        return tabla
    if nodo.izq:
        generar_codigos(nodo.izq, prefijo + "0", tabla)
    if nodo.der:
        generar_codigos(nodo.der, prefijo + "1", tabla)
    return tabla

def main():
    N = len(MENSAJE)
    conteo = Counter(MENSAJE)
    ordenados = sorted(conteo.items(), key=lambda x: (-x[1], x[0]))

    print(f"Mensaje M = '{MENSAJE}' (N = {N} caracteres, k = {len(conteo)} simbolos unicos)\n")
    print(f"{'Simbolo':<8} | {'f_i':<5} | {'p_i':<10} | {'-p_i log2(p_i)':<14}")
    print("-" * 46)

    entropia = 0.0
    for ch, f in ordenados:
        p = f / N
        aporte = -p * math.log2(p)
        entropia += aporte
        print(f"'{ch}'      | {f:<5} | {p:<10.4f} | {aporte:<14.4f}")

    print(f"\nEntropia de Shannon H(X) = {entropia:.4f} bits/simbolo\n")

    # Construccion del arbol paso a paso
    heap = []
    orden = 0
    for ch, f in sorted(conteo.items(), key=lambda x: (x[1], x[0])):
        heapq.heappush(heap, NodoHuffman(f, ch, orden=orden))
        orden += 1

    paso = 1
    while len(heap) > 1:
        n1 = heapq.heappop(heap)
        n2 = heapq.heappop(heap)
        nuevo = NodoHuffman(n1.frec + n2.frec, f"({n1.simbolo}+{n2.simbolo})", n1, n2, orden)
        orden += 1
        heapq.heappush(heap, nuevo)
        print(f"Paso {paso:2d}: Unir [{n1.simbolo}:{n1.frec}] + [{n2.simbolo}:{n2.frec}] -> {nuevo.frec}")
        paso += 1

    raiz = heap[0]
    codigos = generar_codigos(raiz)

    print("\nTabla de Codigos Generada:")
    L_prom = 0.0
    bits_totales = 0
    for ch, f in ordenados:
        c = codigos[ch]
        p = f / N
        L_prom += p * len(c)
        bits_totales += f * len(c)
        print(f"'{ch}': f={f:2d}, codigo={c:<6} (|c|={len(c)}), bits={f*len(c)}")

    print(f"\nLongitud promedio L = {L_prom:.4f} bits/simbolo ({bits_totales}/{N})")
    print(f"Desigualdad de Shannon: {entropia:.4f} <= {L_prom:.4f} < {entropia+1:.4f}")

    k = len(conteo)
    bytes_opcion_a = 256 * 4
    bits_opcion_b = (k - 1) * 1 + k * (1 + 8)
    bytes_opcion_b = math.ceil(bits_opcion_b / 8)
    print(f"\nSobrecarga Opcion A (Tabla 256 uint32): {bytes_opcion_a} bytes")
    print(f"Sobrecarga Opcion B (Preorden k={k}):    {bits_opcion_b} bits = {bytes_opcion_b} bytes")

if __name__ == "__main__":
    main()
