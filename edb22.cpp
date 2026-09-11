#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <chrono>
#include <random>
#include <cmath>
#include <iomanip>

// Define tolerância para comparações de ponto flutuante
const double EPS = 1e-9;

struct Point {
    double x, y;
    int id;
};

struct Segment {
    Point p1, p2;
    int id;

    // Retorna o ponto mais à esquerda (ou inferior em caso de empate)
    Point left_point() const {
        if (p1.x < p2.x || (std::abs(p1.x - p2.x) < EPS && p1.y < p2.y))
            return p1;
        return p2;
    }

    // Retorna o ponto mais à direita
    Point right_point() const {
        if (p1.x < p2.x || (std::abs(p1.x - p2.x) < EPS && p1.y < p2.y))
            return p2;
        return p1;
    }

    // Calcula a coordenada Y do segmento dada uma linha de varredura X
    double get_y(double x) const {
        if (std::abs(p1.x - p2.x) < EPS) return p1.y; // Segmento vertical
        return p1.y + (p2.y - p1.y) * (x - p1.x) / (p2.x - p1.x);
    }
};

// --- Teste de Interseção Básico Geometrico ---
double cross_product(Point a, Point b, Point c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool on_segment(Point a, Point b, Point c) {
    return c.x >= std::min(a.x, b.x) - EPS && c.x <= std::max(a.x, b.x) + EPS &&
           c.y >= std::min(a.y, b.y) - EPS && c.y <= std::max(a.y, b.y) + EPS;
}

bool do_intersect(const Segment& s1, const Segment& s2) {
    Point p1 = s1.p1, q1 = s1.p2;
    Point p2 = s2.p1, q2 = s2.p2;

    double o1 = cross_product(p1, q1, p2);
    double o2 = cross_product(p1, q1, q2);
    double o3 = cross_product(p2, q2, p1);
    double o4 = cross_product(p2, q2, q1);

    // Caso Geral
    if (((o1 > EPS && o2 < -EPS) || (o1 < -EPS && o2 > EPS)) &&
        ((o3 > EPS && o4 < -EPS) || (o3 < -EPS && o4 > EPS)))
        return true;

    // Casos Especiais (colineares)
    if (std::abs(o1) < EPS && on_segment(p1, q1, p2)) return true;
    if (std::abs(o2) < EPS && on_segment(p1, q1, q2)) return true;
    if (std::abs(o3) < EPS && on_segment(p2, q2, p1)) return true;
    if (std::abs(o4) < EPS && on_segment(p2, q2, q1)) return true;

    return false;
}

// ============================================================================
// 1. ALGORITMO DE FORÇA BRUTA - O(n^2)
// ============================================================================
bool intersect_brute_force(const std::vector<Segment>& segments) {
    size_t n = segments.size();
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            if (do_intersect(segments[i], segments[j])) {
                return true;
            }
        }
    }
    return false;
}

// ============================================================================
// 2. ALGORITMO SHAMOS-HOEY - O(n log n)
// ============================================================================
// Detecta se existe pelo menos uma interseção entre n segmentos.
//
// Ideia:
//
// 1. Ordenamos os 2n endpoints pela coordenada x.
// 2. Mantemos uma árvore AVL com os segmentos atualmente ativos.
// 3. A árvore mantém os segmentos ordenados verticalmente na posição
//    atual da linha de varredura.
// 4. Ao inserir um segmento, verificamos seus vizinhos acima e abaixo.
// 5. Ao remover um segmento, verificamos se os vizinhos acima e abaixo
//    passam a se intersectar.
//
// Complexidade:
//     Ordenação dos eventos: O(n log n)
//     2n operações na árvore: O(n log n)
//     Total: O(n log n)
// ============================================================================

double sweep_x = 0.0;

// ============================================================================
// EVENTOS
// ============================================================================

enum EventType {
    START,
    END
};

struct Event {
    double x;
    EventType type;
    int segment_index;

    bool operator<(const Event& other) const {

        if (std::abs(x - other.x) > EPS)
            return x < other.x;

        // Em caso de mesmo x:
        //
        // START antes de END.
        //
        // Isso é importante para detectar segmentos que possuem
        // uma extremidade em comum.
        if (type != other.type)
            return type < other.type;

        return segment_index < other.segment_index;
    }
};

// ============================================================================
// POSIÇÃO Y DE UM SEGMENTO NA LINHA DE VARREDURA
// ============================================================================

double y_at(const Segment& s, double x)
{
    // Segmento vertical.
    //
    // Para o Shamos-Hoey, precisamos de uma ordenação determinística.
    // Usamos o menor y como referência.
    if (std::abs(s.p1.x - s.p2.x) < EPS) {
        return std::min(s.p1.y, s.p2.y);
    }

    return s.p1.y +
           (s.p2.y - s.p1.y) *
           (x - s.p1.x) /
           (s.p2.x - s.p1.x);
}

// ============================================================================
// COMPARAÇÃO DOS SEGMENTOS NA POSIÇÃO ATUAL DA SWEEP LINE
// ============================================================================

bool segment_less(const Segment& a, const Segment& b)
{
    double ya = y_at(a, sweep_x);
    double yb = y_at(b, sweep_x);

    if (std::abs(ya - yb) > EPS)
        return ya < yb;

    // Desempate determinístico.
    return a.id < b.id;
}

// ============================================================================
// NÓ DA ÁRVORE AVL
// ============================================================================

struct AVLNode {

    int segment_index;

    AVLNode* left;
    AVLNode* right;

    int height;

    AVLNode(int index)
        : segment_index(index),
          left(nullptr),
          right(nullptr),
          height(1)
    {}
};

// ============================================================================
// ALTURA
// ============================================================================

int height(AVLNode* node)
{
    return node ? node->height : 0;
}

// ============================================================================
// ATUALIZA ALTURA
// ============================================================================

void update_height(AVLNode* node)
{
    if (!node)
        return;

    node->height =
        1 + std::max(
                height(node->left),
                height(node->right)
            );
}

// ============================================================================
// FATOR DE BALANCEAMENTO
// ============================================================================

int balance_factor(AVLNode* node)
{
    if (!node)
        return 0;

    return height(node->left) -
           height(node->right);
}

// ============================================================================
// ROTAÇÃO À DIREITA
// ============================================================================

AVLNode* rotate_right(AVLNode* y)
{
    AVLNode* x = y->left;
    AVLNode* T2 = x->right;

    x->right = y;
    y->left = T2;

    update_height(y);
    update_height(x);

    return x;
}

// ============================================================================
// ROTAÇÃO À ESQUERDA
// ============================================================================

AVLNode* rotate_left(AVLNode* x)
{
    AVLNode* y = x->right;
    AVLNode* T2 = y->left;

    y->left = x;
    x->right = T2;

    update_height(x);
    update_height(y);

    return y;
}

// ============================================================================
// BALANCEAMENTO
// ============================================================================

AVLNode* balance(AVLNode* node)
{
    if (!node)
        return nullptr;

    update_height(node);

    int bf = balance_factor(node);

    // Caso esquerda-esquerda
    if (bf > 1 &&
        balance_factor(node->left) >= 0)
    {
        return rotate_right(node);
    }

    // Caso esquerda-direita
    if (bf > 1 &&
        balance_factor(node->left) < 0)
    {
        node->left = rotate_left(node->left);
        return rotate_right(node);
    }

    // Caso direita-direita
    if (bf < -1 &&
        balance_factor(node->right) <= 0)
    {
        return rotate_left(node);
    }

    // Caso direita-esquerda
    if (bf < -1 &&
        balance_factor(node->right) > 0)
    {
        node->right = rotate_right(node->right);
        return rotate_left(node);
    }

    return node;
}

// ============================================================================
// INSERÇÃO AVL
// ============================================================================

AVLNode* avl_insert(
    AVLNode* root,
    int segment_index,
    const std::vector<Segment>& segments)
{
    if (!root)
        return new AVLNode(segment_index);

    const Segment& current = segments[segment_index];
    const Segment& root_segment = segments[root->segment_index];

    if (segment_less(current, root_segment)) {

        root->left =
            avl_insert(
                root->left,
                segment_index,
                segments
            );

    } else {

        root->right =
            avl_insert(
                root->right,
                segment_index,
                segments
            );
    }

    return balance(root);
}

// ============================================================================
// MENOR NÓ
// ============================================================================

AVLNode* avl_minimum(AVLNode* node)
{
    AVLNode* current = node;

    while (current && current->left)
        current = current->left;

    return current;
}

// ============================================================================
// REMOÇÃO AVL
// ============================================================================

AVLNode* avl_remove(
    AVLNode* root,
    int segment_index,
    const std::vector<Segment>& segments)
{
    if (!root)
        return nullptr;

    const Segment& target =
        segments[segment_index];

    const Segment& current =
        segments[root->segment_index];

    if (segment_less(target, current)) {

        root->left =
            avl_remove(
                root->left,
                segment_index,
                segments
            );

    }
    else if (segment_less(current, target)) {

        root->right =
            avl_remove(
                root->right,
                segment_index,
                segments
            );

    }
    else {

        // Encontramos o nó.

        if (!root->left || !root->right) {

            AVLNode* temp =
                root->left ?
                root->left :
                root->right;

            if (!temp) {

                delete root;
                return nullptr;

            } else {

                AVLNode* old = root;
                root = temp;

                delete old;
            }

        } else {

            // Dois filhos.
            //
            // Substitui pelo sucessor.

            AVLNode* temp =
                avl_minimum(root->right);

            root->segment_index =
                temp->segment_index;

            root->right =
                avl_remove(
                    root->right,
                    temp->segment_index,
                    segments
                );
        }
    }

    return balance(root);
}

// ============================================================================
// BUSCA DO SEGMENTO
// ============================================================================

AVLNode* avl_find(
    AVLNode* root,
    int segment_index,
    const std::vector<Segment>& segments)
{
    if (!root)
        return nullptr;

    const Segment& target =
        segments[segment_index];

    const Segment& current =
        segments[root->segment_index];

    if (target.id == current.id)
        return root;

    if (segment_less(target, current))
        return avl_find(
            root->left,
            segment_index,
            segments
        );

    return avl_find(
        root->right,
        segment_index,
        segments
    );
}

// ============================================================================
// PREDECESSOR
// Retorna o segmento imediatamente abaixo de target.
// ============================================================================

int avl_predecessor(
    AVLNode* root,
    int segment_index,
    const std::vector<Segment>& segments)
{
    int predecessor = -1;

    while (root) {

        const Segment& current =
            segments[root->segment_index];

        const Segment& target =
            segments[segment_index];

        if (segment_less(current, target)) {

            predecessor =
                root->segment_index;

            root = root->right;

        } else {

            root = root->left;
        }
    }

    return predecessor;
}

// ============================================================================
// SUCESSOR
// Retorna o segmento imediatamente acima de target.
// ============================================================================

int avl_successor(
    AVLNode* root,
    int segment_index,
    const std::vector<Segment>& segments)
{
    int successor = -1;

    while (root) {

        const Segment& current =
            segments[root->segment_index];

        const Segment& target =
            segments[segment_index];

        if (segment_less(target, current)) {

            successor =
                root->segment_index;

            root = root->left;

        } else {

            root = root->right;
        }
    }

    return successor;
}

// ============================================================================
// LIBERA ÁRVORE
// ============================================================================

void delete_tree(AVLNode* root)
{
    if (!root)
        return;

    delete_tree(root->left);
    delete_tree(root->right);

    delete root;
}

// ============================================================================
// SHAMOS-HOEY
// ============================================================================

bool intersect_shamos_hoey(
    const std::vector<Segment>& segments)
{
    int n =
        static_cast<int>(segments.size());

    if (n < 2)
        return false;

    // ------------------------------------------------------------------------
    // EVENTOS
    // ------------------------------------------------------------------------

    std::vector<Event> events;

    events.reserve(2 * n);

    for (int i = 0; i < n; ++i) {

        Point left =
            segments[i].left_point();

        Point right =
            segments[i].right_point();

        events.push_back({
            left.x,
            START,
            i
        });

        events.push_back({
            right.x,
            END,
            i
        });
    }

    // ------------------------------------------------------------------------
    // ORDENA EVENTOS
    // ------------------------------------------------------------------------

    std::sort(
        events.begin(),
        events.end()
    );

    // ------------------------------------------------------------------------
    // ÁRVORE DE STATUS
    // ------------------------------------------------------------------------

    AVLNode* root = nullptr;

    // ------------------------------------------------------------------------
    // PROCESSA EVENTOS
    // ------------------------------------------------------------------------

    for (const Event& event : events) {

        sweep_x = event.x;

        int id =
            event.segment_index;

        // ====================================================================
        // START
        // ====================================================================

        if (event.type == START) {

            // ---------------------------------------------------------------
            // Insere na árvore.
            // ---------------------------------------------------------------

            root =
                avl_insert(
                    root,
                    id,
                    segments
                );

            // ---------------------------------------------------------------
            // Procura vizinho abaixo.
            // ---------------------------------------------------------------

            int below =
                avl_predecessor(
                    root,
                    id,
                    segments
                );

            // ---------------------------------------------------------------
            // Procura vizinho acima.
            // ---------------------------------------------------------------

            int above =
                avl_successor(
                    root,
                    id,
                    segments
                );

            // ---------------------------------------------------------------
            // Testa abaixo.
            // ---------------------------------------------------------------

            if (below != -1) {

                if (do_intersect(
                        segments[id],
                        segments[below]))
                {
                    delete_tree(root);
                    return true;
                }
            }

            // ---------------------------------------------------------------
            // Testa acima.
            // ---------------------------------------------------------------

            if (above != -1) {

                if (do_intersect(
                        segments[id],
                        segments[above]))
                {
                    delete_tree(root);
                    return true;
                }
            }
        }

        // ====================================================================
        // END
        // ====================================================================

        else {

            // ---------------------------------------------------------------
            // Antes de remover o segmento, encontramos os dois vizinhos.
            // ---------------------------------------------------------------

            int below =
                avl_predecessor(
                    root,
                    id,
                    segments
                );

            int above =
                avl_successor(
                    root,
                    id,
                    segments
                );


            // ---------------------------------------------------------------
            // Se existirem os dois vizinhos, eles se tornarão adjacentes
            // depois da remoção.
            // ---------------------------------------------------------------

            if (below != -1 &&
                above != -1)
            {
                if (do_intersect(
                        segments[below],
                        segments[above]))
                {
                    delete_tree(root);
                    return true;
                }
            }


            // ---------------------------------------------------------------
            // Remove o segmento.
            // ---------------------------------------------------------------

            root =
                avl_remove(
                    root,
                    id,
                    segments
                );
        }
    }
    // ------------------------------------------------------------------------
    // Nenhuma interseção encontrada.
    // ------------------------------------------------------------------------

    delete_tree(root);

    return false;
}

// ============================================================================
// GERADOR DE ENTRADAS (FORA DA MEDIÇÃO)
// ============================================================================
std::vector<Segment> gerar_segmentos(int n, uint32_t seed) {
    std::vector<Segment> segments;
    segments.reserve(n);

    // Cria 'n' segmentos horizontais paralelos empilhados no eixo Y
    // Como são paralelos, NUNCA se cruzam, forçando o Força Bruta a testar N*(N-1)/2 vezes.
    for (int i = 0; i < n; ++i) {
        double y = i * 10.0;
        double x1 = 0.0;
        double x2 = 100.0;

        segments.push_back({ {x1, y, i}, {x2, y, i}, i });
    }
    return segments;
}

// ============================================================================
// EXPERIMENTO PRINCIPAL
// ============================================================================
int main() {
    
    std::cout << "=========================================================\n";
    std::cout << " EXPERIMENTO EMPIRICO: FORCA BRUTA vs SHAMOS-HOEY       \n";
    std::cout << "=========================================================\n\n";

    // Tamanhos de N para testar a curva de crescimento
    std::vector<int> tamanhos_n = {1000, 2000, 4000, 8000, 16000};
    int num_repeticoes = 5; // Repetições para tirar a média

    std::cout << "N_Segmentos\tTempo_FB_ms\tTempo_SH_ms\tInt_FB | Int_SH\n";
    std::cout << "---------------------------------------------------------\n";

    for (int n : tamanhos_n) {
        double tempo_fb_acumulado = 0.0;
        double tempo_sh_acumulado = 0.0;
        bool result_fb = false, result_sh = false;

        for (int r = 0; r < num_repeticoes; ++r) {
            // 1. GERA A ENTRADA (FORA DO CRONÔMETRO)
            auto entradas = gerar_segmentos(n, 12345 + r);

            // 2. CRONOMETRA FORÇA BRUTA O(n^2)
            auto t1 = std::chrono::high_resolution_clock::now();
            result_fb = intersect_brute_force(entradas);
            auto t2 = std::chrono::high_resolution_clock::now();
            tempo_fb_acumulado += std::chrono::duration<double, std::milli>(t2 - t1).count();

            // 3. CRONOMETRA SHAMOS-HOEY O(n log n)
            auto t3 = std::chrono::high_resolution_clock::now();
            result_sh = intersect_shamos_hoey(entradas);
            auto t4 = std::chrono::high_resolution_clock::now();
            tempo_sh_acumulado += std::chrono::duration<double, std::milli>(t4 - t3).count();
        }

        double media_fb = tempo_fb_acumulado / num_repeticoes;
        double media_sh = tempo_sh_acumulado / num_repeticoes;

        std::cout << n << "\t\t" 
                  << media_fb << "\t\t" 
                  << media_sh << "\t\t"
                  << (result_fb ? "SIM" : "NAO") << " | " 
                  << (result_sh ? "SIM" : "NAO") << "\n";
    }

    std::cout << "---------------------------------------------------------\n";
    return 0;
}