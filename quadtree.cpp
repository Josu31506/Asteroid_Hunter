#include "quadtree.h"

QuadTree::QuadTree(const CustomRectangle& b, int cap, int mD, int d)
    : boundary(b), capacity(cap), maxDepth(mD), currentDepth(d), objects(cap), divided(false),
      nw(nullptr), ne(nullptr), sw(nullptr), se(nullptr) {}

QuadTree::~QuadTree() { clear(); }

void QuadTree::subdivide() {
    float x = boundary.x;
    float y = boundary.y;
    float w = boundary.width / 2.0f;
    float h = boundary.height / 2.0f;

    // Los centros de los nuevos cuadrantes se calculan desplazando w/4 desde el centro original
    nw = new QuadTree(CustomRectangle(x - w/2, y - h/2, w, h), capacity, maxDepth, currentDepth + 1);
    ne = new QuadTree(CustomRectangle(x + w/2, y - h/2, w, h), capacity, maxDepth, currentDepth + 1);
    sw = new QuadTree(CustomRectangle(x - w/2, y + h/2, w, h), capacity, maxDepth, currentDepth + 1);
    se = new QuadTree(CustomRectangle(x + w/2, y + h/2, w, h), capacity, maxDepth, currentDepth + 1);

    divided = true;

    // RE-INSERTAR objetos del nodo actual en los hijos (Opcional pero recomendado para precisión)
    for (int i = 0; i < objects.size(); i++) {
        GameObject obj = objects.get(i);
        nw->insert(obj);
        ne->insert(obj);
        sw->insert(obj);
        se->insert(obj);
    }
    objects.clear();
}

bool QuadTree::insert(const GameObject& object) {
    // Si el objeto no está ni siquiera cerca de este cuadrante, salir
    if (!boundary.intersects(object.getBounds())) return false;

    if (!divided) {
        // Si hay espacio o llegamos al límite de profundidad, guardar aquí
        if (objects.size() < capacity || currentDepth >= maxDepth) {
            objects.add(object);
            return true;
        }
        subdivide();
    }

    // IMPORTANTE: Se intenta insertar en TODOS los hijos.
    // Un objeto puede vivir en varios cuadrantes si está en la frontera.
    bool i1 = nw->insert(object);
    bool i2 = ne->insert(object);
    bool i3 = sw->insert(object);
    bool i4 = se->insert(object);

    return (i1 || i2 || i3 || i4);
}

void QuadTree::query(const CustomRectangle& range, GameObjectList& foundList) const {
    if (!boundary.intersects(range)) return;

    if (!divided) {
        for (int i = 0; i < objects.size(); i++) {
            if (range.intersects(objects.get(i).getBounds())) {
                foundList.add(objects.get(i));
            }
        }
    } else {
        nw->query(range, foundList);
        ne->query(range, foundList);
        sw->query(range, foundList);
        se->query(range, foundList);
    }
}

void QuadTree::clear() {
    objects.clear();
    if (divided) {
        delete nw; delete ne; delete sw; delete se;
        nw = ne = sw = se = nullptr;
        divided = false;
    }
}