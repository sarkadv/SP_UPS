from constants import *


# graf pro spravu propojeni tlacitek
class Graph:
    # inicializace grafu, matice sousednosti na 0
    def __init__(self, vertex_count: int):
        self.matrix = [[0 for _ in range(vertex_count)] for _ in range(vertex_count)]
        self.vertex_count = vertex_count
        self.sqrt_vertex = DOTS

    # pridani hrany, jde o neorientovany graf
    def add_edge(self, x: int, y: int):
        self.matrix[x][y] = 1
        self.matrix[y][x] = 1

    # vrati seznam sousedu vrcholu (vrcholu, do kterych z tohoto vrcholu vedou hrany)
    def neighbors(self, vertex: int):
        neighbors = []

        for i in range(self.vertex_count):
            if self.matrix[vertex][i] == 1:
                neighbors.append(i)

        return neighbors

    # vrati boolean podle toho, jestli jsou dva vrcholy propojene
    def is_neighbor(self, v1: int, v2: int):
        return self.matrix[v1][v2] == 1

    # vypise na konzoli matici sousednosti
    def print(self):
        for x in range(self.vertex_count):
            for y in range(self.vertex_count):
                print(str(self.matrix[x][y]) + " ", end='')
            print("")

    # najde nove vytvorene vrcholy, ve kterych zacina cyklus delky 4 (hotovy ctverec)
    def find_new_cycle_vertices(self, current_vertices1: list[int], current_vertices2: list[int]):
        new_vertices = []

        for x in range(self.vertex_count - self.sqrt_vertex):     # bez posledniho radku
            if (x + 1) % self.sqrt_vertex == 0:                   # bez posledniho sloupce
                continue
            if self.is_neighbor(x, x+1):
                if self.is_neighbor(x+1, x+1+self.sqrt_vertex):
                    if self.is_neighbor(x+1+self.sqrt_vertex, x+self.sqrt_vertex):
                        if self.is_neighbor(x+self.sqrt_vertex, x):
                            if (not (x in current_vertices1)) and (not (x in current_vertices2)):
                                new_vertices.append(x)

        return new_vertices
