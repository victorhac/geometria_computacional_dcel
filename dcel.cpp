#include <iostream>
#include <vector>
#include <GL/glut.h>
#include <math.h>

#define THRESHOLD 0.05
#define PRODUTO_VETORIAL_THRESHOLD 0.01
#define WIDTH 600
#define HEIGHT 400

struct Point
{
    float x, y;
    Point(float x, float y) : x(x), y(y) {}
};

struct Vertex
{
    Point point;
    struct HalfEdge *incidentEdge;
    Vertex(float x, float y) : point(x, y), incidentEdge(nullptr) {}
};

int edge_number = 0;

struct HalfEdge
{
    int number;
    Vertex *origin;
    HalfEdge *twin;
    HalfEdge *next;
    HalfEdge *prev;
    struct Face *incidentFace;
    HalfEdge(Vertex *origin) : origin(origin), twin(nullptr), next(nullptr), prev(nullptr), incidentFace(nullptr)
    {
        number = ++edge_number;
    }
};

int face_number = 0;

struct Face
{
    int number;
    HalfEdge *outerComponent;
    Face() : outerComponent(nullptr)
    {
        number = ++face_number;
    }
};

enum State
{
    SHOW_FACE_EDGES,
    VERTEX_ORBIT,
    EDGE_INCLUSION,
    VERTEX_INCLUSION
};

State state = EDGE_INCLUSION;

struct DCEL
{
    std::vector<Vertex *> vertices;
    std::vector<HalfEdge *> edges;
    std::vector<Face *> faces;
};

DCEL dcel;

std::vector<Vertex *> changed_vertices;
Vertex *last_vertice = nullptr;

std::vector<HalfEdge*> edges_face;
Vertex* vertex_face = nullptr;

std::vector<HalfEdge*> edges_orbit;

float produto_vetorial(Vertex *p1, Vertex *p2, Vertex *p3)
{
    float vetor_a_x = p1->point.x - p2->point.x;
    float vetor_a_y = p1->point.y - p2->point.y;
    float vetor_b_x = p3->point.x - p2->point.x;
    float vetor_b_y = p3->point.y - p2->point.y;
    
    return vetor_b_x * vetor_a_y - vetor_b_y * vetor_a_x;
}

bool is_left_on(Vertex *vertex, Vertex *origin, Vertex *destination)
{
    float resultadoProdutoVetorial = produto_vetorial(vertex, origin, destination);
    return resultadoProdutoVetorial >= 0.0;
}

float get_angle(HalfEdge *halfEdge1, HalfEdge *halfEdge2)
{
    float vetor1x = halfEdge1->twin->origin->point.x - halfEdge1->origin->point.x;
    float vetor1y = halfEdge1->twin->origin->point.y - halfEdge1->origin->point.y;
    float vetor2x = halfEdge2->twin->origin->point.x - halfEdge2->origin->point.x;
    float vetor2y = halfEdge2->twin->origin->point.y - halfEdge2->origin->point.y;

    float produtoEscalar = (vetor1x * vetor2x) + (vetor1y * vetor2y);

    float magnitudeVetor1 = std::sqrt((vetor1x * vetor1x) + (vetor1y * vetor1y));
    float magnitudeVetor2 = std::sqrt((vetor2x * vetor2x) + (vetor2y * vetor2y));

    float cosenoAngulo = produtoEscalar / (magnitudeVetor1 * magnitudeVetor2);

    float anguloRadianos = std::acos(cosenoAngulo);

    return anguloRadianos * (180.0 / M_PI);
}

HalfEdge *get_prev_edge(HalfEdge *next)
{
    HalfEdge *he_left = nullptr;
    HalfEdge *he_right = nullptr;

    float left_angle;
    float right_angle;

    for (HalfEdge *e : dcel.edges)
    {
        HalfEdge *considered_edge = nullptr;

        if (e->twin != next && e->twin->origin == next->origin)
        {
            considered_edge = e;
        }
        else if (e != next && e->origin == next->origin)
        {
            considered_edge = e->twin;
        }

        if (considered_edge != nullptr)
        {
            float angle = get_angle(considered_edge->twin, next);
            if (is_left_on(considered_edge->origin, next->origin, next->twin->origin)) //
            {
                if (he_left == nullptr || angle < left_angle)
                {
                    he_left = considered_edge;
                    left_angle = angle;
                }
            }
            else
            {
                if (he_right == nullptr || angle > right_angle)
                {
                    he_right = considered_edge;
                    right_angle = angle;
                }
            }
        }
    }

    return he_left != nullptr ? he_left : he_right != nullptr ? he_right : nullptr;
}

Face *create_face()
{
    Face *f = new Face();
    dcel.faces.push_back(f);
    return f;
}

HalfEdge *create_edge(Vertex *v1, Vertex *v2)
{
    HalfEdge *e = new HalfEdge(v1);
    HalfEdge *et = new HalfEdge(v2);

    v1->incidentEdge = e;
    v2->incidentEdge = et;

    e->twin = et;
    et->twin = e;
    e->next = et;
    et->next = e;
    e->prev = et;
    et->prev = e;

    dcel.edges.push_back(e);

    if (dcel.edges.size() == 1)
    {
        Face *f = create_face();
        f->outerComponent = e;

        e->incidentFace = f;
        et->incidentFace = f;
    }

    return e;
}

Vertex *create_vertex(float x, float y)
{
    Vertex *v = new Vertex(x, y);
    dcel.vertices.push_back(v);
    return v;
}

bool is_clockwise_oriented(const std::vector<Vertex*> polygon) {
    int n = polygon.size();
    float area = 0.0;

    for (int i = 0; i < n; ++i) {
        int j = (i + 1) % n;
        area += (polygon[i]->point.x * polygon[j]->point.y) - (polygon[j]->point.x * polygon[i]->point.y);
    }

    return area > 0;
}

bool connection_generated_cicle(HalfEdge *first)
{
    HalfEdge *previous = first;
    HalfEdge *e = first->next;

    std::vector<Vertex*> polygon;

    polygon.push_back(first->origin);

    do
    {
        if (e == previous || e == previous->twin || e == first->twin)
            return false;

        polygon.push_back(e->origin);

        previous = e;
        e = e->next;
    } while (e != first);

    return is_clockwise_oriented(polygon);
}

void update_incident_face_cicle(HalfEdge *first)
{
    Face* face = first->incidentFace;

    HalfEdge *e = first;
    HalfEdge *outer_component = nullptr;

    do
    {
        if (e->twin->incidentFace == face)
        {
            outer_component = e->twin;
            break;
        }

        e = e->next;
    } while (e != first);

    if (outer_component == nullptr)
        return;
        
    Face *new_face = create_face();
    new_face->outerComponent = first;
    face->outerComponent = outer_component;

    e = first;

    do
    {
        e->incidentFace = new_face;
        e = e->next;
    } while (e != first);
}

void connect_orbit(HalfEdge *e1, HalfEdge *e2)
{
    e1->next = e2;
    e2->prev = e1;
}

void connect(Vertex *a, Vertex *b)
{
    HalfEdge *e3 = create_edge(a, b);

    HalfEdge *e1 = get_prev_edge(e3);
    HalfEdge *e2 = get_prev_edge(e3->twin);

    if (e1 == nullptr)
        e1 = e3->twin;

    if (e2 == nullptr)
        e2 = e3;

    e3->incidentFace = e1->incidentFace;
    e3->twin->incidentFace = e2->incidentFace;

    e3->twin->next = e1->next;
    e1->next->prev = e3->twin;

    e3->next = e2->next;
    e2->next->prev = e3;

    connect_orbit(e1, e3);
    connect_orbit(e2, e3->twin);

    if (connection_generated_cicle(e3))
        update_incident_face_cicle(e3);

    if (connection_generated_cicle(e3->twin))
        update_incident_face_cicle(e3->twin);
}

HalfEdge *get_edge_by_vertices(Vertex *a, Vertex *b)
{
    HalfEdge *he = nullptr;

    for (HalfEdge *e : dcel.edges)
    {
        if (e->origin == a && e->twin->origin == b)
        {
            he = e;
            break;
        } else if (e->twin->origin == a && e->origin == b)
        {
            he = e->twin;
            break;
        }
    }

    return he;
}

void clear()
{
    dcel.vertices.clear();
    dcel.edges.clear();
    dcel.faces.clear();
}

bool are_close_by_threshold(float a, float b, float threshold)
{
    return (a >= b - threshold && a <= b + threshold);
}

bool are_close(float a, float b)
{
    return are_close_by_threshold(a, b, THRESHOLD);
}

Vertex *get_vertex_by_coordinates(float x, float y)
{
    for (Vertex *v : dcel.vertices)
        if (are_close(v->point.x, x) && are_close(v->point.y, y))
            return v;

    return nullptr;
}

void edge_inclusion_mode(float x, float y)
{
    if (get_vertex_by_coordinates(x, y) == nullptr) {
        create_vertex(x, y);
        return;
    }

    if (last_vertice == nullptr)
    {
        last_vertice = get_vertex_by_coordinates(x, y);
    }
    else if (last_vertice != get_vertex_by_coordinates(x, y))
    {
        Vertex *v1 = last_vertice;
        Vertex *v2 = get_vertex_by_coordinates(x, y);

        if (dcel.edges.size() > 0)
            connect(v1, v2);
        else
            create_edge(v1, v2);

        last_vertice = nullptr;
    }
}

void show_face_edges_mode(float x, float y) {
    if (vertex_face == nullptr) {
        vertex_face = get_vertex_by_coordinates(x, y);
        return;
    }

    Vertex* v = get_vertex_by_coordinates(x, y);
    HalfEdge* first = get_edge_by_vertices(vertex_face, v);

    edges_face.clear();
    
    HalfEdge* e = first;
    do {
        edges_face.push_back(e);
        e = e->next;
    } while (e != first);

    vertex_face = nullptr;
}

void vertex_orbit_mode(float x, float y) {
    Vertex* v = get_vertex_by_coordinates(x, y);

    edges_orbit.clear();

    HalfEdge* first = v->incidentEdge;
    HalfEdge* e = first->prev;

    edges_orbit.push_back(first);

    int contador = 0;

    while (e != first && e != first->twin) {
        if (contador % 2 == 0) {
            e = e->twin;
        } else {
            edges_orbit.push_back(e);
            e = e->prev;
        }
        contador++;
    }
}

void vertex_inclusion_mode(float x, float y) {
    HalfEdge* edge = nullptr;
    Vertex* vertex = create_vertex(x, y);

    for (HalfEdge* e : dcel.edges) {
        if (are_close_by_threshold(produto_vetorial(vertex, e->origin, e->twin->origin), 0, PRODUTO_VETORIAL_THRESHOLD)) {
            edge = e;
            break;
        }
    }

    HalfEdge* new_edge = create_edge(vertex, edge->twin->origin);
    edge->twin->origin = vertex;

    HalfEdge* next = edge->next;
    HalfEdge* twin_prev = edge->twin->prev;

    new_edge->next = next;
    next->prev = new_edge;
    new_edge->twin->prev = twin_prev;
    twin_prev->next = new_edge->twin;

    new_edge->prev = edge;
    edge->next = new_edge;
    new_edge->twin->next = edge->twin;
    edge->twin->prev = new_edge->twin;

    new_edge->incidentFace = edge->incidentFace;
    new_edge->twin->incidentFace = edge->twin->incidentFace;
}

void mouse(int button, int state_button, int coord_x, int coord_y)
{
    if (button == GLUT_LEFT_BUTTON && state_button == GLUT_DOWN)
    {
        float x = float(coord_x) * 2 / WIDTH - 1;
        float y = -(float(coord_y) * 2 / HEIGHT - 1);

        switch (state)
        {
            case SHOW_FACE_EDGES:
                show_face_edges_mode(x, y);
                break;
            case VERTEX_ORBIT:
                vertex_orbit_mode(x, y);
                break;
            case EDGE_INCLUSION:
                edge_inclusion_mode(x, y);
                break;
            case VERTEX_INCLUSION:
                vertex_inclusion_mode(x, y);
                break;
        }

        glutPostRedisplay();
    }
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1.0, 1.0, 1.0);

    glPointSize(10.0);
    glBegin(GL_POINTS);
    for (Vertex *v : dcel.vertices)
    {
        glVertex2f(v->point.x, v->point.y);
    }

    glEnd();

    glBegin(GL_LINES);
    for (HalfEdge *e : dcel.edges)
    {
        glVertex2f(e->origin->point.x, e->origin->point.y);
        glVertex2f(e->twin->origin->point.x, e->twin->origin->point.y);
    }
    glEnd();

    glFlush();
}

void display_edge_highlighted(HalfEdge *highlighted_edge)
{
    display();

    glColor3f(1.0, 0, 0);
    glBegin(GL_LINES);

    glVertex2f(highlighted_edge->origin->point.x, highlighted_edge->origin->point.y);
    glVertex2f(highlighted_edge->twin->origin->point.x, highlighted_edge->twin->origin->point.y);

    glEnd();
    glFlush();
}

int display_highlighted(int i)
{
    switch (state)
    {
        case SHOW_FACE_EDGES:
            if (edges_face.size() > 1) {
                display_edge_highlighted(edges_face[i % edges_face.size()]);
                return 1;
            }
            return 0;
        case VERTEX_ORBIT:
            if (edges_orbit.size() > 1) {
                display_edge_highlighted(edges_orbit[i % edges_orbit.size()]);
                return 1;
            }
            return 0;
    }

    return 0;
}

const int desiredFPS = 60;
const int frameDelay = 50000 / desiredFPS;

int prevFrameTime = 0;
int counter = 0;

void update() {
    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    int elapsedTime = currentTime - prevFrameTime;

    if (elapsedTime >= frameDelay) {
        counter += display_highlighted(counter);
        prevFrameTime = currentTime;
    }
}

void keyboard(unsigned char key, int x, int y)
{
    edges_face.clear();
    edges_orbit.clear();
    vertex_face = nullptr;
    last_vertice = nullptr;
    counter = 0;
    
    switch (key)
    {
    case 'c':
        clear();
        glutPostRedisplay();
        break;
    case 'f':
        state = SHOW_FACE_EDGES;
        break;
    case 'o':
        state = VERTEX_ORBIT;
        break;
    case 'e':
        state = EDGE_INCLUSION;
        break;
    case 'v':
        state = VERTEX_INCLUSION;
        break;
    }
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("DCEL");

    glutDisplayFunc(display);
    glutIdleFunc(update);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);

    glutMainLoop();

    for (Vertex *v : dcel.vertices)
        delete v;
    for (HalfEdge *e : dcel.edges)
    {
        delete e->twin;
        delete e;
    }
    for (Face *f : dcel.faces)
        delete f;

    return 0;
}
