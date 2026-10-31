#ifndef RED_BLACK_TREE_PRACTICE_H_
#define RED_BLACK_TREE_PRACTICE_H_

// =============================================================================
// 红黑树 · 练习版（关键逻辑留空，注释就是路标）
// -----------------------------------------------------------------------------
// 写代码前后都请默念红黑树口诀：
//
//   1. 根叶黑  —— 根结点是黑色；所有"叶子"（这里的 NIL 哨兵）是黑色
//   2. 不红红  —— 红色结点的孩子必是黑色（树上不允许出现相邻的两个红）
//   3. 黑路同  —— 从任意结点出发到它的每一个 NIL 叶子，路径上黑色结点个数相同
//
// 外加一条查找树的本分（与颜色无关）：BST 有序，左 < 我 < 右。
// validate() 会替你逐条检查这四条，违反了还会告诉你是哪条被破坏。
//
// -----------------------------------------------------------------------------
// 两个关键的设计选择（先读懂，再动手）：
//
//   [1] 结点带 parent 指针（三叉链表）
//       插入修复时需要频繁"看父亲、看爷爷、看叔叔"。没有 parent 指针，
//       就得一路用栈回溯，代码立刻复杂一截。
//
//   [2] 用唯一的 nil_ 哨兵代替 nullptr 表示"空孩子"（CLRS《算法导论》风格）
//       所有叶子与空孩子共用同一个黑色结点。好处有二：
//         - "叶子是黑"这条口诀由哨兵天然保证（它生来就是黑的）；
//         - 修复逻辑里少一大半空指针特判（叔叔不存在 = 叔叔是黑的哨兵）。
//
//   [3]（思考题答案先不写）新结点一律先染红，为什么？—— 见 TODO④ 的注释。
//
// -----------------------------------------------------------------------------
// 你的任务：按顺序补全 6 个 TODO（都标了 [YOUR CODE HERE]）：
//
//   TODO① findNode_ + contains   热身：体会"红黑树的查找 = 普通 BST 查找"
//   TODO② leftRotate             左旋（小心 parent 指针！）
//   TODO③ rightRotate            右旋（左旋的镜像）
//   TODO④ insert                 BST 下潜 + 挂上新结点
//   TODO⑤ insertFixup            重头戏：三情况修复
//   TODO⑥ erase / transplant / eraseFixup   进阶选做，真难，卡住就看答案
//
// 随时可以运行 ../run_tests.sh（Windows cmd 下运行 build.bat）自测。
// 测试会自动探测你完成到哪一步，没写完的部分会友好跳过。
// =============================================================================

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

template <typename T, typename Compare = std::less<T>>
class RedBlackTree {
 private:
  // 结点与颜色的定义放在类最前面：后面成员函数的参数类型要用到它们。
  // 想一想：为什么每个结点都要存 parent、color？（答案在文件头注释 [1][2]）
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
    // 哨兵 nil_：一颗"公共的黑叶子"。它的 key 无意义（用默认值占位）。
    // 注意它的 left/right/parent 都指向自己，这样旋转等代码不会踩到空指针。
    nil_ = new Node(T{}, Color::BLACK, nullptr, nullptr, nullptr);
    nil_->left = nil_;
    nil_->right = nil_;
    nil_->parent = nil_;
    root_ = nil_;  // 空树：根就是哨兵（根是黑 —— 口诀第一条从构造起就成立）
  }

  ~RedBlackTree() {
    destroy(root_);
    delete nil_;
  }

  RedBlackTree(const RedBlackTree&) = delete;             // 练习用，禁掉拷贝
  RedBlackTree& operator=(const RedBlackTree&) = delete;  // （深拷贝不是本课重点）

  // ---------------- 已提供的基础设施（不用改，测试要用） ----------------
  std::size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }

  // 插入至今引发的"单次旋转"总次数（一次双旋 = 2）。
  // 测试里用它和 AVL 对比，体会红黑树"更新更省"的优点。旋转写完后别删计数行。
  int rotations() const { return rotations_; }

  // 树高（按结点数计：空树 0，只有根 1）。用来和 2*log2(n+1) 的理论上界比。
  int height() const { return height_(root_); }

  // 黑高：根到 NIL 的路径上黑色内部结点数。口诀"黑路同"保证了各条路径一样长，
  // 所以随便沿左路数就行。黑高 ≤ log2(n+1)，而树高 ≤ 2 * 黑高 —— 树不高的根源。
  int blackHeight() const {
    int bh = 0;
    for (Node* n = root_; n != nil_; n = n->left)
      if (n->color == Color::BLACK) ++bh;
    return bh;
  }

  // 中序遍历收集结果（应该是有序的 —— BST 本分，测试会检查）
  void inorder(std::vector<T>& out) const { inorderCollect_(root_, out); }
  std::vector<T> inorder() const {
    std::vector<T> out;
    out.reserve(size_);
    inorderCollect_(root_, out);
    return out;
  }

  // 侧向打印（右子树在上，缩进越深层越深），每个结点标 [R]/[B]。
  // 50 个结点以内肉眼检查"不红红"非常直观，调试 insertFixup 的好帮手。
  void draw() const { draw_(root_, 0); }

  // 口诀检查器（已完整提供，是测试的基石）。
  // 用法：std::string err; if (!t.validate(&err)) { /* err 里写着违反了哪条 */ }
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

  // ---------------- 你的战场（TODO 区） ----------------

  // TODO①（热身）查找：红黑树的查找逻辑与普通 BST 一模一样，没有任何特殊处理！
  //   平衡不是查找代码给的，是树形保证的 —— 这正是"查找和 AVL 没有本质区别"的原因。
  //   提示：从 root_ 出发，key 小走左、大走右；用 comp_(a, b) 表示 a < b。
  //   判等没有 ==，用"互不小于"：!comp_(a, b) && !comp_(b, a)。
  //   先把它写成私有函数 findNode_（TODO⑥ erase 也要复用），再让 contains 调它。
  Node* findNode_(const T& key) const {
    // [YOUR CODE HERE]
    (void)key;
    return nil_;  // 占位：实现后删除
  }

  bool contains(const T& key) const {
    // [YOUR CODE HERE]  一行调用 findNode_ 即可
    (void)key;
    return false;  // 占位：实现后删除
  }

  // TODO② 左旋。这是红黑树唯一的"物理搬动"，20 行以内，唯一难点是 parent 指针。
  //
  //       x                 y      |
  //      / \               / \      |
  //     A   y     ==>     x   C     |
  //        / \           / \       |
  //       B   C         A   B      |
  //
  //   步骤（按序写，每步都在动指针）：
  //     1. y = x->right（y 不可能是 nil_，调用方保证）
  //     2. 把 y 的左孩子 B 过继给 x 当右孩子（B 可能是 nil_，过继前判一下！）
  //     3. 让 y 顶替 x 的位置：y->parent = x->parent；
  //        若 x 原本是根（x->parent == nil_）→ root_ = y；
  //        否则按 x 是父的左孩子还是右孩子，把 y 接上去
  //     4. 最后把 x 接到 y 的左边，别忘了 x->parent = y
  void leftRotate(Node* x) {
    // [YOUR CODE HERE]
    (void)x;
  }

  // TODO③ 右旋：左旋的镜像（left/right 全部互换），不再重复提示。
  //
  //        y              x       |
  //       / \            / \      |
  //      x   C     ==>    A   y   |
  //     / \                  / \  |
  //    A   B                B   C |
  //
  void rightRotate(Node* x) {
    // [YOUR CODE HERE]
    (void)x;
  }

  // TODO④ 插入第一步：像普通 BST 一样下潜，挂上新结点。
  //   步骤：
  //     a) cur 从 root_ 出发向下走，parent 跟在后面记录"将来的父亲"；
  //     b) 途中遇到相等的 key → return false（本练习不允许重复）；
  //     c) 到底后 new Node(key, Color::RED, parent, nil_, nil_),
  //        按 key 与 parent 的大小挂在左边或右边；空树时它就是新根；
  //     d) ++size_; 然后调用 insertFixup(z)。
  //
  //   ★ 思考：新结点为什么必须染红？
  //     染黑 → 每条经过它的路径都多了一个黑结点 → 立刻破坏【黑路同】，
  //     而且没有局部办法补救；染红 → 最多只破坏【不红红】，一种违规好修。
  bool insert(const T& key) {
    // [YOUR CODE HERE]
    (void)key;
    return false;  // 占位：实现后删除
  }

  // TODO⑤（重头戏）插入修复：新结点 z 是红的，若它的父亲也是红的 → 违反【不红红】。
  //
  //   循环条件：while (z->parent 是红)。父是黑就直接收工（树此时合法）。
  //   父是红 ⇒ 父一定不是根 ⇒ 爷爷一定存在（且是黑的）。
  //   下面以"父是爷爷的左孩子"为例（镜像情形把 left/right 全互换即可）：
  //
  //   口诀：叔叔红 → 变一变；叔叔黑 → 转一转。
  //
  //   情况1（叔叔是红）：只变色，不上转。父、叔变黑，爷爷变红，
  //        然后 z = 爷爷，继续向上检查（红爷爷可能又撞上红曾祖）。
  //
  //        g(黑)               g(红)    |
  //       /  \                /  \      |     z 跳到爷爷身上，
  //     p(红) u(红)   ==>   p(黑) u(黑)  |     循环继续向上检查
  //     /                   /           |
  //   z(红)               z(红)         |
  //
  //   情况2（叔叔黑，且 z 是父亲的右孩子 —— "内侧"）：先对父亲左旋，
  //        把形状扭成情况3，然后按情况3处理（注意旋转后 z 的父亲变了，要重新取！）。
  //
  //        g(黑)             g(黑)      |
  //       /                 /           |
  //     p(红)       ==>   z(红)          |  ← 现在变成"外侧"形状了
  //       \               /              |
  //        z(红)         p(红)           |
  //
  //   情况3（叔叔黑，且 z 是父亲的左孩子 —— "外侧"）：父变黑、爷变红，
  //        对爷爷右旋。完成后父(黑)接管了这块子树，循环可以结束。
  //
  //        g(黑)               p(黑)    |
  //       / \                 / \       |
  //     p(红) u(黑)   ==>   z(红) g(红)  |
  //     /                             \ |
  //   z(红)                           u(黑)
  //
  //   全部修完后，最后一行：把根染黑（口诀【根叶黑】——情况1可能把根染成了红）。
  void insertFixup(Node* z) {
    // [YOUR CODE HERE]
    (void)z;
  }

  // TODO⑥（进阶选做）删除。建议先跑通前五个 TODO，回头再来挑战。
  //
  //   总路线（CLRS）：
  //   1. findNode_ 定位 z；z == nil_ → return false。
  //   2. 若 z 有两个孩子：找右子树的最小结点 y（后继）来"接班"，
  //      记下 y 原来的颜色（它才是真正离开树的那个结点）；否则 y 就是 z 自己。
  //   3. 用 transplant(u, v) 把 v 搬到 u 的位置上；
  //      x 是接替 y 的孩子（可能是 nil_ 哨兵 —— 所以哨兵的 parent 必须可写，
  //      这不是 bug，是 fixup 要顺着它找父亲）。
  //   4. 如果真正离开的 y 原本是黑的 → 黑路上少了一个黑，必须 eraseFixup(x) 补偿。
  //      （这就是删除难的原因：插入只会"多一个红"，删除却会"少一个黑"。）
  //   5. eraseFixup 的四情形（x 是左孩子时，镜像对称）：
  //        i.    兄弟红                → 变色 + 旋父，转成"兄弟黑"
  //        ii.   兄弟黑、两侄都黑      → 兄弟染红，把"补偿"上移一层（x = 父）
  //        iii.  兄弟黑、近侄红远侄黑  → 旋兄弟，变成情形 iv
  //        iv.   兄弟黑、远侄红        → 旋父，父的黑分给兄弟，结束
  //      收尾把 x 染黑（吸收多余的"黑"）。
  bool erase(const T& key) {
    // [YOUR CODE HERE]
    (void)key;
    return false;  // 占位：实现后删除
  }

  void transplant(Node* u, Node* v) {
    // [YOUR CODE HERE]  v 接管 u 的位置（v 可以是 nil_ 哨兵）
  }

  void eraseFixup(Node* x) {
    // [YOUR CODE HERE]
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

  // ---------- validate 的检查器（已完整提供） ----------
  static bool fail_(std::string* err, const std::string& msg) {
    if (err) *err = msg;
    return false;
  }

  // 一趟 DFS 同时查：BST 有序、parent 指针一致、【不红红】
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

  // 口诀【黑路同】：每个结点左右子树的黑高必须相等；返回黑高，-1 表示已报错
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

#endif  // RED_BLACK_TREE_PRACTICE_H_
