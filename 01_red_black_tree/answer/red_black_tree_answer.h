#ifndef RED_BLACK_TREE_ANSWER_H_
#define RED_BLACK_TREE_ANSWER_H_

// =============================================================================
// 红黑树 · 参考答案（接口与 practice/red_black_tree.h 完全一致）
// -----------------------------------------------------------------------------
// 使用方法：卡住时对照对应函数看"为什么这么写"；看完合上，自己再写一遍。
// 用 ./run_tests.sh answer 可以验证这份答案本身通过全部测试。
// =============================================================================

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

template <typename T, typename Compare = std::less<T>>
class RedBlackTree {
 private:
  // 结点与颜色的定义放在类最前面：后面成员函数的参数类型要用到它们
  enum class Color : unsigned char { RED, BLACK };

  struct Node {
    T key;
    Color color;
    Node* parent;
    Node* left;
    Node* right;
    Node(const T& k, Color c, Node* p, Node* l, Node* r)
        : key(k), color(c), parent(p), left(l), right(r) {}
  };

 public:
  RedBlackTree() : comp_() {
    // 哨兵 nil_：所有"叶子"和"空孩子"都共用它。它是黑的 —— 口诀【根叶黑】
    // 的"叶黑"一半由构造保证，一半由它保证。
    nil_ = new Node(T{}, Color::BLACK, nullptr, nullptr, nullptr);
    nil_->left = nil_;
    nil_->right = nil_;
    nil_->parent = nil_;
    root_ = nil_;
  }

  ~RedBlackTree() {
    destroy(root_);
    delete nil_;
  }

  RedBlackTree(const RedBlackTree&) = delete;
  RedBlackTree& operator=(const RedBlackTree&) = delete;

  std::size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }
  int rotations() const { return rotations_; }
  int height() const { return height_(root_); }

  int blackHeight() const {
    int bh = 0;
    for (Node* n = root_; n != nil_; n = n->left)
      if (n->color == Color::BLACK) ++bh;
    return bh;
  }

  void inorder(std::vector<T>& out) const { inorderCollect_(root_, out); }
  std::vector<T> inorder() const {
    std::vector<T> out;
    out.reserve(size_);
    inorderCollect_(root_, out);
    return out;
  }

  void draw() const { draw_(root_, 0); }

  bool validate(std::string* err = nullptr) const {
    if (root_ != nil_ && root_->parent != nil_)
      return fail_(err, "结构错误: root_->parent 应该是 nil_ 哨兵");
    if (root_->color != Color::BLACK)
      return fail_(err, "口诀【根叶黑】被破坏: 根结点不是黑色");
    if (nil_->color != Color::BLACK)
      return fail_(err, "口诀【根叶黑】被破坏: NIL 哨兵不是黑色");
    if (!checkOrderAndNoRedRed_(root_, nullptr, nullptr, err)) return false;
    return checkBlackHeight_(root_, err) >= 0;
  }

  // ------------------------- 查找 -------------------------
  // 与普通 BST 一字不差。红黑树对查找的全部贡献是"树形不会退化"：
  // 黑路同 ⇒ 黑高 ≤ log2(n+1) ⇒ 高度 ≤ 2·log2(n+1)，最坏情形也是 O(log n)。
  // （AVL 更矮一点，单次查找常数略小 —— 这是 AVL 的理论优势，见测试第 5 节。）
  Node* findNode_(const T& key) const {
    Node* cur = root_;
    while (cur != nil_) {
      if (!comp_(key, cur->key) && !comp_(cur->key, key))  // 互不小于 = 相等
        return cur;
      cur = comp_(key, cur->key) ? cur->left : cur->right;
    }
    return nil_;
  }

  bool contains(const T& key) const { return findNode_(key) != nil_; }

  // ------------------------- 旋转 -------------------------
  // 左旋：x 和它的右孩子 y "换座"，y 的左子树 B 过继给 x。
  // 注意旋转不改变任何路径的黑色结点数（x、y 互换位置但没变色），
  // 所以它只用来调整形状，颜色问题交给 fixup。
  void leftRotate(Node* x) {
    ++rotations_;  // 实验统计用（与 AVL 对比），请保留
    Node* y = x->right;          // y 一定会顶上来（调用方保证 y != nil_）
    x->right = y->left;          // B 过继给 x
    if (y->left != nil_) y->left->parent = x;
    y->parent = x->parent;       // y 接管 x 的位置
    if (x->parent == nil_)
      root_ = y;                 // x 原本是根
    else if (x == x->parent->left)
      x->parent->left = y;
    else
      x->parent->right = y;
    y->left = x;                 // x 落到 y 左下
    x->parent = y;
  }

  // 右旋：左旋的镜像。
  void rightRotate(Node* x) {
    ++rotations_;
    Node* y = x->left;
    x->left = y->right;
    if (y->right != nil_) y->right->parent = x;
    y->parent = x->parent;
    if (x->parent == nil_)
      root_ = y;
    else if (x == x->parent->right)
      x->parent->right = y;
    else
      x->parent->left = y;
    y->right = x;
    x->parent = y;
  }

  // ------------------------- 插入 -------------------------
  // 两阶段：① BST 下潜挂上新红结点；② insertFixup 修复【不红红】。
  bool insert(const T& key) {
    Node* parent = nil_;  // 跟在 cur 身后，记住"将来的父亲"
    Node* cur = root_;
    while (cur != nil_) {
      parent = cur;
      if (!comp_(key, cur->key) && !comp_(cur->key, key))
        return false;  // 已存在，本练习不收重复
      cur = comp_(key, cur->key) ? cur->left : cur->right;
    }
    Node* z = new Node(key, Color::RED, parent, nil_, nil_);
    if (parent == nil_)
      root_ = z;
    else if (comp_(key, parent->key))
      parent->left = z;
    else
      parent->right = z;
    ++size_;
    insertFixup(z);
    return true;
  }

  // 修复【不红红】。只可能"新红结点撞上红父亲"。
  // 情况1（叔叔红）只变色并把问题上推一层 —— O(log n) 次变色；
  // 情况2/3（叔叔黑）旋转 —— 每次插入最多 1 次双旋 + 1 次单旋 = 2 次旋转。
  // 这就是"红黑树插入的旋转是 O(1)"的出处，也是它比 AVL 删除好养的原因之一。
  void insertFixup(Node* z) {
    while (z->parent->color == Color::RED) {  // 父黑 → 天下太平
      Node* p = z->parent;
      Node* g = p->parent;  // 父是红 ⇒ 父不是根 ⇒ 爷爷必存在（黑）
      if (p == g->left) {
        Node* u = g->right;  // 叔叔
        if (u->color == Color::RED) {
          // 情况1：红父红叔 → 父叔染黑、爷染红，z 跳到爷爷继续向上
          p->color = Color::BLACK;
          u->color = Color::BLACK;
          g->color = Color::RED;
          z = g;
        } else {
          // 情况2：z 是"内侧"（父的右孩子）→ 先左旋父亲扭成"外侧"
          if (z == p->right) {
            z = p;            // 左旋后原来的父亲沉到左下
            leftRotate(z);
            p = z->parent;    // ★ 旋转后父亲换了人，必须重新取
          }
          // 情况3：外侧 → 父染黑、爷染红、右旋爷爷，结束
          p->color = Color::BLACK;
          g->color = Color::RED;
          rightRotate(g);
        }
      } else {
        // 镜像：父是爷爷的右孩子，left/right 全互换
        Node* u = g->left;
        if (u->color == Color::RED) {
          p->color = Color::BLACK;
          u->color = Color::BLACK;
          g->color = Color::RED;
          z = g;
        } else {
          if (z == p->left) {
            z = p;
            rightRotate(z);
            p = z->parent;
          }
          p->color = Color::BLACK;
          g->color = Color::RED;
          leftRotate(g);
        }
      }
    }
    root_->color = Color::BLACK;  // 口诀【根叶黑】：情况1可能把根染红了
  }

  // ------------------------- 删除 -------------------------
  // 把 v 接到 u 的位置上（v 可以是 nil_ 哨兵 —— 此时 v->parent 被临时写入，
  // 这是有意的：eraseFixup 要顺着 x->parent 找到父亲）。
  void transplant(Node* u, Node* v) {
    if (u->parent == nil_)
      root_ = v;
    else if (u == u->parent->left)
      u->parent->left = v;
    else
      u->parent->right = v;
    v->parent = u->parent;
  }

  bool erase(const T& key) {
    Node* z = findNode_(key);
    if (z == nil_) return false;

    Node* y = z;            // y：真正离开树的结点（z 或 z 的后继）
    Color yOrig = y->color; // 记住它离开前的颜色 —— 决定要不要修复
    Node* x;                // 接替 y 的孩子

    if (z->left == nil_) {
      x = z->right;
      transplant(z, z->right);
    } else if (z->right == nil_) {
      x = z->left;
      transplant(z, z->left);
    } else {
      // 两孩子：用后继 y 顶替 z 的位置（直接搬结点而非拷贝 key，更简单）
      y = minNode_(z->right);
      yOrig = y->color;
      x = y->right;
      if (y->parent == z) {
        x->parent = y;  // x 可能是 nil_，先挂好，fixup 要用
      } else {
        transplant(y, y->right);
        y->right = z->right;
        y->right->parent = y;
      }
      transplant(z, y);
      y->left = z->left;
      y->left->parent = y;
      y->color = z->color;  // y 披上 z 的颜色，上面的世界毫无察觉
    }
    delete z;
    --size_;
    if (yOrig == Color::BLACK)
      eraseFixup(x);  // 少了一个黑结点 → 必须补偿
    return true;
  }

  // 删除修复：x 这条路上"少了一个黑"（想象 x 背着一个额外的黑包袱）。
  // 目标：要么把包袱甩给一个红结点（染黑收工），要么一路推到根。
  void eraseFixup(Node* x) {
    while (x != root_ && x->color == Color::BLACK) {
      if (x == x->parent->left) {
        Node* w = x->parent->right;  // 兄弟
        if (w->color == Color::RED) {
          // 情况 i：兄弟红 → 兄弟染黑、父染红，左旋父 → 变成"兄弟黑"
          w->color = Color::BLACK;
          x->parent->color = Color::RED;
          leftRotate(x->parent);
          w = x->parent->right;
        }
        if (w->left->color == Color::BLACK && w->right->color == Color::BLACK) {
          // 情况 ii：兄弟黑、两侄黑 → 兄弟染红（卸掉它的黑补偿邻居），
          // 包袱上移一层（x = 父），继续循环
          w->color = Color::RED;
          x = x->parent;
        } else {
          if (w->right->color == Color::BLACK) {
            // 情况 iii：近侄红、远侄黑 → 右旋兄弟，把红侄换成"远侄"
            w->left->color = Color::BLACK;
            w->color = Color::RED;
            rightRotate(w);
            w = x->parent->right;
          }
          // 情况 iv：远侄红 → 左旋父，父的黑分给兄弟，包袱卸掉，结束
          w->color = x->parent->color;
          x->parent->color = Color::BLACK;
          w->right->color = Color::BLACK;
          leftRotate(x->parent);
          x = root_;
        }
      } else {
        // 镜像：x 是右孩子
        Node* w = x->parent->left;
        if (w->color == Color::RED) {
          w->color = Color::BLACK;
          x->parent->color = Color::RED;
          rightRotate(x->parent);
          w = x->parent->left;
        }
        if (w->right->color == Color::BLACK && w->left->color == Color::BLACK) {
          w->color = Color::RED;
          x = x->parent;
        } else {
          if (w->left->color == Color::BLACK) {
            w->right->color = Color::BLACK;
            w->color = Color::RED;
            leftRotate(w);
            w = x->parent->left;
          }
          w->color = x->parent->color;
          x->parent->color = Color::BLACK;
          w->left->color = Color::BLACK;
          rightRotate(x->parent);
          x = root_;
        }
      }
    }
    x->color = Color::BLACK;  // 红 x 直接吸收包袱；推到根则全局减一，皆合法
  }

  void clear() {
    destroy(root_);
    root_ = nil_;
    size_ = 0;
    rotations_ = 0;
  }

 private:
  Node* root_ = nullptr;
  Node* nil_ = nullptr;
  std::size_t size_ = 0;
  int rotations_ = 0;
  Compare comp_{};

  Node* minNode_(Node* n) const {
    while (n->left != nil_) n = n->left;
    return n;
  }

  int height_(Node* n) const {
    return n == nil_ ? 0 : 1 + std::max(height_(n->left), height_(n->right));
  }

  void inorderCollect_(Node* n, std::vector<T>& out) const {
    if (n == nil_) return;
    inorderCollect_(n->left, out);
    out.push_back(n->key);
    inorderCollect_(n->right, out);
  }

  void destroy(Node* n) {
    if (n == nil_) return;
    destroy(n->left);
    destroy(n->right);
    delete n;
  }

  void draw_(Node* n, int depth) const {
    if (n == nil_) return;
    draw_(n->right, depth + 1);
    std::cout << std::string(static_cast<std::size_t>(depth) * 4, ' ')
              << n->key << (n->color == Color::RED ? " [R]" : " [B]") << "\n";
    draw_(n->left, depth + 1);
  }

  static bool fail_(std::string* err, const std::string& msg) {
    if (err) *err = msg;
    return false;
  }

  bool checkOrderAndNoRedRed_(Node* n, const T* lo, const T* hi,
                              std::string* err) const {
    if (n == nil_) return true;
    if (lo && !comp_(*lo, n->key))
      return fail_(err, "BST 有序被破坏: 存在 key 不满足 左 < 结点 < 右");
    if (hi && !comp_(n->key, *hi))
      return fail_(err, "BST 有序被破坏: 存在 key 不满足 左 < 结点 < 右");
    if (n->left != nil_ && n->left->parent != n)
      return fail_(err, "结构错误: 某结点的 left->parent 指错了");
    if (n->right != nil_ && n->right->parent != n)
      return fail_(err, "结构错误: 某结点的 right->parent 指错了");
    if (n->color == Color::RED &&
        (n->left->color == Color::RED || n->right->color == Color::RED))
      return fail_(err, "口诀【不红红】被破坏: 存在相邻的红-红结点");
    return checkOrderAndNoRedRed_(n->left, lo, &n->key, err) &&
           checkOrderAndNoRedRed_(n->right, &n->key, hi, err);
  }

  int checkBlackHeight_(Node* n, std::string* err) const {
    if (n == nil_) return 0;
    int lh = checkBlackHeight_(n->left, err);
    if (lh < 0) return -1;
    int rh = checkBlackHeight_(n->right, err);
    if (rh < 0) return -1;
    if (lh != rh) {
      fail_(err, "口诀【黑路同】被破坏: 到不同 NIL 的路径黑色结点数不同");
      return -1;
    }
    return lh + (n->color == Color::BLACK ? 1 : 0);
  }
};

#endif  // RED_BLACK_TREE_ANSWER_H_
