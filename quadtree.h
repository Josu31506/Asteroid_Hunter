#ifndef QUADTREE_H
#define QUADTREE_H

struct Point {
    float x, y;
    Point(float _x = 0, float _y = 0) : x(_x), y(_y) {}
};

struct CustomRectangle {
    float x, y; // Centro
    float width, height;
    CustomRectangle(float _x = 0, float _y = 0, float _w = 0, float _h = 0)
        : x(_x), y(_y), width(_w), height(_h) {}

    bool intersects(const CustomRectangle& range) const {
        return !(range.x - range.width/2 > x + width/2 ||
                 range.x + range.width/2 < x - width/2 ||
                 range.y - range.height/2 > y + height/2 ||
                 range.y + range.height/2 < y - height/2);
    }
};

struct GameObject {
    Point position;
    float width, height;
    int type; // 1: Nave, 2: Asteroide, 3: Bala
    int id;   // Crucial para identificar objetos únicos

    GameObject() : position(0,0), width(0), height(0), type(0), id(-1) {}

    CustomRectangle getBounds() const {
        // Aumentamos un poco el margen para que la colisión sea más permisiva
        return CustomRectangle(position.x, position.y, width, height);
    }
};

class GameObjectList {
private:
    GameObject* data;
    int capacity;
    int currentSize;
    void grow() {
        capacity *= 2;
        GameObject* newData = new GameObject[capacity];
        for (int i = 0; i < currentSize; i++) newData[i] = data[i];
        delete[] data;
        data = newData;
    }
public:
    GameObjectList(int initialCap = 4) : capacity(initialCap), currentSize(0) {
        data = new GameObject[capacity];
    }
    ~GameObjectList() { delete[] data; }
    void add(const GameObject& obj) {
        if (currentSize == capacity) grow();
        data[currentSize++] = obj;
    }
    int size() const { return currentSize; }
    GameObject& get(int index) const { return data[index]; }
    void clear() { currentSize = 0; }
};

class QuadTree {
private:
    CustomRectangle boundary;
    int capacity;
    int maxDepth;
    int currentDepth;
    GameObjectList objects;
    bool divided;
    QuadTree *nw, *ne, *sw, *se;
    void subdivide();
public:
    QuadTree(const CustomRectangle& b, int cap = 50, int mD = 8, int d = 0);
    ~QuadTree();
    bool insert(const GameObject& object);
    void query(const CustomRectangle& range, GameObjectList& foundList) const;
    void clear();
};

#endif