#ifndef AVL_TREE_H_
#define AVL_TREE_H_

// =============================================================================
// AVL 树 · 对照参考实现（完整可用，不需要你写）
// -----------------------------------------------------------------------------
// 作用：给测试第 5 节当"对照组"，用同一批数据量出红黑树与 AVL 的差别：
//   - AVL 平衡标准更严（每个结点左右高度差 ≤ 1）→ 树更矮 → 查找常数略小；
//   - 但代价是维护更勤：同样的插入序列，旋转次数通常比红黑树多。
// 接口与红黑树练习版对齐（insert/contains/size/height/rotations），方便对比。
// =============================================================================

#include <algorithm>
#include <cstddef>

template <typename T, typename Compare = std::less<T>>
class AvlTree {
 public:
  AvlTree() = default;
  ~AvlTree() { destroy(root_); }

  AvlTree(const AvlTree&) = delete;
  AvlTree& operator=(const AvlTree&) = delete;

  bool insert(const T& key) {
    bool inserted = false;
    root_ = insertRec_(root_, key, inserted);
    if (inserted) ++size_;
    return inserted;
  }

  bool contains(const T& key) const {
    Node* cur = root_;
    while (cur) {
      if (!comp_(key, cur->key) && !comp_(cur->key, key)) return true;
      cur = comp_(key, cur->key) ? cur->left : cur->right;
    }
    return false;
  }

  std::size_t size() const { return size_; }
  int height() const { return height_(root_); }
  int rotations() const { return rotations_; }

  void clear() {
    destroy(root_);
    root_ = nullptr;
    size_ = 0;
    rotations_ = 0;
  }

 private:
  struct Node {
    T key;
    Node* left = nullptr;
    Node* right = nullptr;
    int h = 1;  // 以此结点为根的子树高度（叶子为 1）
    explicit Node(const T& k) : key(k) {}
  };

  Node* root_ = nullptr;
  std::size_t size_ = 0;
  int rotations_ = 0;
  Compare comp_{};

  static int height_(Node* n) { return n ? n->h : 0; }
  int balance_(Node* n) const { return height_(n->left) - height_(n->right); }
  static void update_(Node* n) {
    n->h = 1 + std::max(height_(n->left), height_(n->right));
  }

  Node* rotateRight_(Node* y) {
    ++rotations_;
    Node* x = y->left;
    y->left = x->right;
    x->right = y;
    update_(y);
    update_(x);
    return x;  // 新子树根
  }

  Node* rotateLeft_(Node* x) {
    ++rotations_;
    Node* y = x->right;
    x->right = y->left;
    y->left = x;
    update_(x);
    update_(y);
    return y;
  }

  // 插入后沿途回溯 rebalance：一次插入最多触发"单旋+双旋"各一次，
  // 但触发频率高 —— 对比实验里可以看到总数明显多于红黑树。
  Node* rebalance_(Node* n) {
    update_(n);
    int b = balance_(n);
    if (b > 1) {  // 左重
      if (balance_(n->left) < 0) n->left = rotateLeft_(n->left);  // LR → 先左
      return rotateRight_(n);
    }
    if (b < -1) {  // 右重
      if (balance_(n->right) > 0) n->right = rotateRight_(n->right);  // RL
      return rotateLeft_(n);
    }
    return n;
  }

  Node* insertRec_(Node* n, const T& key, bool& inserted) {
    if (!n) {
      inserted = true;
      return new Node(key);
    }
    if (!comp_(key, n->key) && !comp_(n->key, key)) return n;  // 已存在
    if (comp_(key, n->key))
      n->left = insertRec_(n->left, key, inserted);
    else
      n->right = insertRec_(n->right, key, inserted);
    return rebalance_(n);
  }

  void destroy(Node* n) {
    if (!n) return;
    destroy(n->left);
    destroy(n->right);
    delete n;
  }
};

#endif  // AVL_TREE_H_
