#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Struktura reprezentujici jeden tok
// Obsahuje vsechny metriky, ktere se pouzivaji ve vypoctu vzdalenosti
typedef struct {
    int id;       // identifikator toku
    int b;        // total_bytes (prenesena data - bajty)
    int t;        // flow_duration (doba trvani toku)
    int d;        // avg_interarrival_time (prum. doba mezi pakety)
    double s;     // avg_packet_size (prum. delka paketu)
} Flow;

// Struktura obsahujici vahy pro jednotlive atributy toku
// a pozadovany pocet vyslednych shluku
typedef struct {
    int N;        // cilovy pocet shluku
    double wb;    // vaha pro total_bytes
    double wt;    // vaha pro flow_duration
    double wd;    // vaha pro avg_interarrival_time
    double ws;    // vaha pro avg_packet_size
} FlowWeights;

// Struktura pro jeden shluk
typedef struct {
    int *items;   // seznam ID toku patricich do shluku
    int size;     // pocet prvku v tomto shluku
} Cluster;

int clustering;   // shlukovani zapnuto (1) ci vypnuto (0)
int count;        // pocet nactenych toku ze vstupniho souboru

// Cte argumenty programu, nastavuje clustering a vahy
int read_arguments(int argc, char **argv, int *clustering, FlowWeights *fws)
{
    // Povoleny 2 nebo 7 argumentu
    if (argc != 2 && argc != 7) {
        fprintf(stderr, "Error: Incorrect number of arguments provided\n"
                        "Usage: ./flows FILE [N WB WT WD WS]\n"
                        "Usage without clustering: ./flows FILE\n");
        return 0;
    }

    // Pokud je vlozen jen FILE, clustering se nevykonava
    if (argc == 2) {
        *clustering = 0;
        return 1;
    } else {
        *clustering = 1;
    }

    // Nacteni hodnot pro clustering
    fws->N  = atoi(argv[2]);
    fws->wb = atof(argv[3]);
    fws->wt = atof(argv[4]);
    fws->wd = atof(argv[5]);
    fws->ws = atof(argv[6]);

    if (fws->N <= 0) {
        fprintf(stderr, "Error: Argument N must be higher than 0\n");
        return 0;
    }

    return 1;
}

// Nacte jednu radku se zaznamem toku
int read_single_flow(FILE *file, Flow *flows, int line)
{
    int flow_id, total_bytes, flow_duration, packet_count;
    double avg_interarrival;

    // %*s preskakuje nepodstatne stringy ze souboru
    if (fscanf(file, "%d %*s %*s %d %d %d %lf",
               &flow_id, &total_bytes, &flow_duration,
               &packet_count, &avg_interarrival) != 5) {
        fprintf(stderr, "Error: Invalid format on line %d in the file\n", line);
        return -1;
    }

    // Naplneni struktury Flow
    flows->id = flow_id;
    flows->b  = total_bytes;
    flows->t  = flow_duration;
    flows->d  = avg_interarrival;
    flows->s  = packet_count ? (double)total_bytes / packet_count : 0.0;

    return 0;
}

// Nacteni vstupniho souboru
Flow *load_flows(char *filename, int *count)
{
    FILE *file = fopen(filename, "r");
    Flow *flows = NULL;

    if (!file) {
        fprintf(stderr, "Error: The file %s not found \n", filename);
        return NULL;
    }

    // Ocekava prvni radek: count=X
    if (fscanf(file, "count=%d", count) != 1 || *count <= 0) {
        fprintf(stderr, "Error: Missing or invalid 'count=' line in the file\n");
        fclose(file);
        return NULL;
    }

    // Alokace pole toku
    flows = malloc(sizeof(Flow) * (*count));
    if (!flows) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    // Nacteme jednotlive toky
    for (int i = 0; i < *count; i++) {
        if (read_single_flow(file, &flows[i], i + 1) != 0) {
            free(flows);
            fclose(file);
            return NULL;
        }
    }
    fclose(file);

    return flows;
}

// Vytvori pro kazdy tok jeden shluk obsahujici pouze jeho ID
Cluster *cluster_ctor(int count, Flow *flows)
{
    Cluster *cls = malloc(sizeof(Cluster) * count);
    if (!cls) return NULL;

    for (int i = 0; i < count; i++) {
        cls[i].size = 1;
        cls[i].items = malloc(sizeof(int));

        if (!cls[i].items) {
            fprintf(stderr, "Error: Memory allocation failed for cluster %d\n", i);
            for (int j = 0; j < i; j++) {
                free(cls[j].items);
            }
            free(cls);
            return NULL;
        }
        cls[i].items[0] = flows[i].id;  // kazdy shluk nese ID jednoho toku
    }

    return cls;
}

// Hledani indexu toku podle ID
int find_flow_index(Flow *flows, int count, int id)
{
    // Prohledava pole toku a hleda konkretni ID
    for (int i = 0; i < count; i++)
        if (flows[i].id == id)
            return i;

    return -1;
}

// Vypocet vzdalenosti dvou toku
double get_flow_distance(Flow *a, Flow *b, FlowWeights *fws)
{
    // Rozdily atributu dvou toku
    double db = a->b - b->b;
    double dt = a->t - b->t;
    double dd = a->d - b->d;
    double ds = a->s - b->s;

    // Vypocet eukleidovy vzdalenosti s vahami
    return sqrt(fws->wb*db*db + fws->wt*dt*dt + 
                fws->wd*dd*dd + fws->ws*ds*ds);
}

// Vzdalenost dvou shluku
double get_clusters_distance(Cluster *a, Cluster *b, Flow *flows,
                             int count, FlowWeights *fws)
{
    double shortest_distance = 1e308; // max. velke cislo

    // Porovna kazdy tok ze shluku A s kazdym tokem ze shluku B
    for (int i = 0; i < a->size; i++) {
        int idx_a = find_flow_index(flows, count, a->items[i]);

        for (int j = 0; j < b->size; j++) {
            int idx_b = find_flow_index(flows, count, b->items[j]);
            double distance = get_flow_distance(&flows[idx_a], &flows[idx_b], fws);

            if (distance < shortest_distance) {
                shortest_distance = distance; // hledame minimum
            }
        }
    }
    
    return shortest_distance;
}

// Slouceni shluku B -> A
void merge_clusters(Cluster *a, Cluster *b)
{
    // Realokace pole pro slouceny shluk
    a->items = realloc(a->items, sizeof(int) * (a->size + b->size));

    // Zkopirovani hodnot ze shluku B
    for (int i = 0; i < b->size; i++)
        a->items[a->size + i] = b->items[i];

    // Aktualizace velikosti
    a->size += b->size;

    // Uvolneni shluku B
    free(b->items);
    b->items = NULL;
    b->size = 0;
}

// Najde dva nejblizsi shluky
void find_closest_clusters(Cluster *cls, int count, Flow *flows,
                           FlowWeights *fws, int *idx_A, int *idx_B)
{
    double shortest_distance = 1e308;

    for (int i = 0; i < count; i++) {
        if (cls[i].size == 0) {
            continue; // preskoc prazdne shluky
        }
        for (int j = i + 1; j < count; j++) {
            if (cls[j].size == 0) {
                continue;
            }
            // vzdalenost shluku i a j
            double distance = get_clusters_distance(&cls[i], &cls[j], 
                                                    flows, count, fws);

            if (distance < shortest_distance) {
                shortest_distance = distance;
                *idx_A = i;
                *idx_B = j;
            }
        }
    }
}

// Vrati indexy aktivnich (neprazdnych) shluku
int *get_active_clusters(Cluster *cls, int count, int *active_cls_out)
{
    int active_cls = 0;

    // Spocitani aktivnich
    for (int i = 0; i < count; i++)
        if (cls[i].size > 0)
            active_cls++;

    *active_cls_out = active_cls;

    // Alokace pole indexu aktivnich shluku
    int *active_cls_idxs = malloc(sizeof(int) * active_cls);

    int idx = 0;
    for (int i = 0; i < count; i++)
        if (cls[i].size > 0) {
            active_cls_idxs[idx] = i;
            idx++;
        }

    return active_cls_idxs;
}

// Najde nejmensi ID ve shluku
int get_min_id(Cluster *cls)
{
    int min_id = cls->items[0];

    for (int i = 1; i < cls->size; i++)
        if (cls->items[i] < min_id) {
            min_id = cls->items[i];
        }

    return min_id;
}

// Razeni shluku podle jejich minimalniho ID
void sort_clusters_by_min_id(Cluster *cls, int *active_cls_idxs,
                             int active_cls)
{
    // bubble sort razeni
    for (int i = 0; i < active_cls; i++) {
        for (int j = i + 1; j < active_cls; j++) {
            int cls_i = active_cls_idxs[i];
            int cls_j = active_cls_idxs[j];

            if (get_min_id(&cls[cls_j]) < get_min_id(&cls[cls_i])) {
                int tmp = active_cls_idxs[i];

                active_cls_idxs[i] = active_cls_idxs[j];
                active_cls_idxs[j] = tmp;
            }
        }
    }
}

// Komparator pro qsort
int compare_int(const void *a, const void *b)
{
    return (*(int*)a) - (*(int*)b);
}

// Tisk jednoho shluku
void print_single_cluster(Cluster *c, int idx)
{
    // Serazeni ID toku uvnitr shluku
    qsort(c->items, c->size, sizeof(int), compare_int);

    printf("cluster %d:", idx);
    for (int i = 0; i < c->size; i++)
        printf(" %d", c->items[i]);
    printf("\n");
}

// Tisk vsech shluku
void print_clusters(Cluster *cls, int count)
{
    int active_cls = 0;
    int *active_cls_idxs = get_active_clusters(cls, count, &active_cls);

    //Serazeni podle nejmensiho ID v shluku
    sort_clusters_by_min_id(cls, active_cls_idxs, active_cls);

    printf("Clusters:\n");
    for (int i = 0; i < active_cls; i++)
        print_single_cluster(&cls[active_cls_idxs[i]], i);

    free(active_cls_idxs);
}

// Hlavni shlukovaci algoritmus
void perform_clustering(Cluster *cls, int count,
                        Flow *flows, FlowWeights *fws)
{
    int current = count;

    while (current > fws->N) {
        int a, b;
        // Najdeme dva nejblizsi shluky
        find_closest_clusters(cls, count, flows, fws, &a, &b);
        
        // Sloucime je
        merge_clusters(&cls[a], &cls[b]);
        current--;
    }
}

// Uvolneni pameti
void free_all(Cluster *cls, int count, Flow *flows)
{
    if (cls) {
        for (int i = 0; i < count; i++)
            free(cls[i].items);
        free(cls);
    }
    free(flows);
}

int main(int argc, char **argv)
{
    FlowWeights fws;
    Flow *flows = NULL;
    Cluster *cls = NULL;

    // Parsovani argumentu
    if (!read_arguments(argc, argv, &clustering, &fws))
        return 1;

    // Nacteni toku ze souboru
    flows = load_flows(argv[1], &count);
    if (!flows) {
        free_all(cls, count, flows);
        return 1;
    }

    // Inicializace shluku
    cls = cluster_ctor(count, flows);
    if (!cls) {
        free_all(cls, count, flows);
        return 1;
    }

    // Pokud je clustering zapnuty, provedeme shlukovani
    if (clustering) {
        if (fws.N > count) {
            fprintf(stderr, "Error: N cannot exceed number of flows\n");
            free_all(cls, count, flows);
            return 1;
        }
        perform_clustering(cls, count, flows, &fws);
    }

    // Vytisknuti vyslednych shluku
    print_clusters(cls, count);

    // Uvolneni pameti
    free_all(cls, count, flows);
    
    return 0;
}
