// =============================================================================
// 红黑树测试套件（无第三方依赖，g++ -std=c++17 直接编译）
// -----------------------------------------------------------------------------
// 用法：
//   测练习版:   ./run_tests.sh            （Windows cmd: build.bat）
//   测参考答案: ./run_tests.sh answer     （验证答案本身全绿）
//   也可以手动编译，命令见项目 README。
//
// 各节考察点（设计思路）：
//   第0节 实现探测     自动判断练习完成度：没写完的部分友好跳过，不刷屏
//   第1节 基础功能     空树、单元素、重复值、string/double 泛型
//   第2节 口诀自检     每插一个数就做一次全树性质校验 —— 口诀被破坏时
//                      输出具体违反了哪一条（根叶黑 / 不红红 / 黑路同 / BST有序）
//   第3节 对抗输入     递增/递减/锯齿插入 —— 朴素 BST 的 O(n) 死穴，
//                      红黑树必须把高度死死压在 2*log2(n+1) 以内
//   第4节 随机压测     两万随机数：全命中、误查必失败、性质不破、中序有序
//   第5节 RB vs AVL    旋转次数 / 树高 / 查找耗时 —— 用数据体会两种平衡树
//                      的取舍（优点用断言锁死，短板用打印观察项）
//   第6节 RB vs 朴素BST 红黑树存在的意义：同样有序插入，一个退化成链表，
//                      一个纹丝不动
//   第7节 删除(进阶)   erase 未实现时自动跳过
// =============================================================================

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#ifdef USE_ANSWER
#include "red_black_tree_answer.h"
#define TEST_TARGET "参考答案 answer/red_black_tree_answer.h"
#else
#include "red_black_tree.h"
#define TEST_TARGET "练习版 practice/red_black_tree.h"
#endif

#include "avl_tree.h"

// ---------------------------- 极简测试框架 ----------------------------
static int gTotal = 0, gPass = 0, gFail = 0, gSkip = 0;

static bool checkImpl(bool ok, int line, const std::string& msg) {
  ++gTotal;
  if (ok) {
    ++gPass;
  } else {
    ++gFail;
    std::printf("  [FAIL] (第%d行) %s\n", line, msg.c_str());
  }
  return ok;
}
#define CHECK(cond, msg) checkImpl(static_cast<bool>(cond), __LINE__, (msg))

static void section(const char* title) {
  std::printf("\n========== %s ==========\n", title);
}

static void skipNote(const char* what) {
  ++gSkip;
  std::printf("  [SKIP] %s\n", what);
}

template <typename Fn>
static double benchMs(Fn&& fn) {
  auto t0 = std::chrono::steady_clock::now();
  fn();
  auto t1 = std::chrono::steady_clock::now();
  return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

// 朴素 BST（对照品）：什么都不平衡，用来演示"红黑树到底解决了什么"
template <typename T>
class NaiveBst {
 public:
  bool insert(const T& key) {
    if (!root_) {
      root_ = new Node{key};
      ++size_;
      return true;
    }
    Node* cur = root_;
    for (;;) {
      if (!(key < cur->key) && !(cur->key < key)) return false;
      Node*& next = key < cur->key ? cur->left : cur->right;
      if (!next) {
        next = new Node{key};
        ++size_;
        return true;
      }
      cur = next;
    }
  }
  bool contains(const T& key) const {
    Node* cur = root_;
    while (cur) {
      if (!(key < cur->key) && !(cur->key < key)) return true;
      cur = key < cur->key ? cur->left : cur->right;
    }
    return false;
  }
  int height() const { return height_(root_); }
  std::size_t size() const { return size_; }

 private:
  struct Node {
    T key;
    Node* left = nullptr;
    Node* right = nullptr;
  };
  static int height_(const Node* n) {
    return n ? 1 + std::max(height_(n->left), height_(n->right)) : 0;
  }
  Node* root_ = nullptr;
  std::size_t size_ = 0;
};

// 插入一段 key 后校验性质；失败时打印第几个数出错（方便定位），
// 并画出一棵小树给肉眼看。返回是否全程合法。
static bool insertAllAndValidate(RedBlackTree<int>& t,
                                 const std::vector<int>& keys,
                                 bool drawOnFail = false) {
  for (std::size_t i = 0; i < keys.size(); ++i) {
    t.insert(keys[i]);
    std::string err;
    if (!t.validate(&err)) {
      CHECK(false, "插入第 " + std::to_string(i + 1) + " 个数 " +
                       std::to_string(keys[i]) + " 之后: " + err);
      if (drawOnFail) t.draw();
      return false;
    }
  }
  return true;
}

// ---------------------------- 第0节 · 实现探测 ----------------------------
// 目的：判断练习完成到哪一步，没完成的部分后面全部友好跳过。
static bool probeInsert() {
  RedBlackTree<int> t;
  bool ok = true;
  ok &= CHECK(t.insert(5), "insert(5) 首次插入应返回 true");
  ok &= CHECK(t.insert(3), "insert(3) 应返回 true");
  ok &= CHECK(t.insert(8), "insert(8) 应返回 true");
  ok &= CHECK(!t.insert(5), "重复 insert(5) 应返回 false");
  ok &= CHECK(t.size() == 3, "重复插入后 size 应为 3");
  std::printf("  [探测] insert（BST 下潜部分）: %s\n",
              ok ? "完成" : "未完成或有 bug");
  return ok;
}

static bool probeContains() {
  RedBlackTree<int> t;
  t.insert(5);
  t.insert(3);
  t.insert(8);
  bool ok = true;
  ok &= CHECK(t.contains(3) && t.contains(5) && t.contains(8), "已插入的都应能查到");
  ok &= CHECK(!t.contains(4) && !t.contains(9), "没插入的不应查到");
  std::printf("  [探测] contains: %s\n", ok ? "完成" : "未完成或有 bug");
  return ok;
}

// 递增插入 1..10：5 个结点起就必然触发旋转和修复，能探出 TODO②③⑤ 是否完成
static bool probeRotateAndFixup() {
  RedBlackTree<int> t;
  for (int k = 1; k <= 10; ++k) t.insert(k);
  std::string err;
  bool ok = t.validate(&err);
  ok &= CHECK(ok, std::string("递增插入 1..10 后性质校验失败: ") +
                      (ok ? "" : err));
  std::vector<int> v = t.inorder();
  ok &= CHECK(v.size() == 10 && std::is_sorted(v.begin(), v.end()),
              "递增插入 1..10 后中序应为 1..10 有序");
  std::printf("  [探测] 旋转 + insertFixup: %s\n", ok ? "完成" : "未完成或有 bug");
  return ok;
}

static bool probeErase() {
  RedBlackTree<int> t;
  for (int k = 1; k <= 5; ++k) t.insert(k);
  bool ok = true;
  ok &= CHECK(t.erase(3), "erase(3) 应返回 true");
  ok &= CHECK(t.size() == 4, "删除后 size 应为 4");
  ok &= CHECK(!t.contains(3), "删除后不应再查到 3");
  std::string err;
  ok &= CHECK(t.validate(&err), std::string("删除后性质校验: ") + (ok && err.empty() ? "通过" : err));
  std::printf("  [探测] erase: %s\n", ok ? "完成" : "未实现或有 bug");
  return ok;
}

// ---------------------------- 第1节 · 基础功能与边界 ----------------------------
static void testBasics() {
  section("第1节 · 基础功能与边界");

  RedBlackTree<int> t;
  CHECK(t.empty() && t.size() == 0, "新树应为空");
  CHECK(!t.contains(1), "空树 contains 应返回 false");
  CHECK(t.height() == 0, "空树高度应为 0");
  std::string err;
  CHECK(t.validate(&err), std::string("空树也应满足全部口诀: ") + err);

  CHECK(t.insert(42), "单元素插入");
  CHECK(!t.empty() && t.size() == 1, "size == 1");
  CHECK(t.height() == 1, "单结点高度应为 1");
  CHECK(t.validate(&err), std::string("单结点校验: ") + err);
  CHECK(!t.insert(42), "重复插入应返回 false 且 size 不变");
  CHECK(t.size() == 1, "重复插入后 size 仍为 1");

  // 小树画出来看看颜色（[R]/[B]），肉眼体会"不红红"
  const int seq[] = {50, 30, 70, 20, 40, 60, 80};
  RedBlackTree<int> t2;
  for (int k : seq) CHECK(t2.insert(k), "批量插入");
  CHECK(t2.size() == 7, "7 个不同元素");
  for (int k : seq) CHECK(t2.contains(k), "已插入的都应查到");
  CHECK(!t2.contains(10) && !t2.contains(65) && !t2.contains(90),
        "未插入的 10/65/90 不应查到");
  std::vector<int> want = {20, 30, 40, 50, 60, 70, 80};
  CHECK(t2.inorder() == want, "中序遍历应恰好有序（BST 本分）");
  CHECK(t2.validate(&err), std::string("7 结点树校验: ") + err);
  std::printf("  ---- 50/30/70/20/40/60/80 依次插入后的树（右子树在上）----\n");
  t2.draw();
  std::printf("  ----------------------------------------------------------\n");

  // 泛型：模板对其他类型同样有效
  RedBlackTree<std::string> ts;
  ts.insert("pear");
  ts.insert("apple");
  ts.insert("fig");
  CHECK(ts.contains("apple") && !ts.contains("banana"), "string 键可用");
  CHECK(ts.validate(&err), "string 键的性质校验");
  RedBlackTree<double> td;
  td.insert(3.14);
  td.insert(2.71);
  td.insert(3.14);
  CHECK(td.size() == 2 && td.contains(2.71), "double 键可用且去重");
}

// ---------------------------- 第2节 · 口诀逐条自检 ----------------------------
static void testMottoEveryInsert() {
  section("第2节 · 口诀自检：每插一个数就查一遍全树性质");
  std::mt19937 rng(42);
  std::vector<int> keys(200);
  std::iota(keys.begin(), keys.end(), 0);
  std::shuffle(keys.begin(), keys.end(), rng);

  RedBlackTree<int> t;
  if (insertAllAndValidate(t, keys)) {
    CHECK(true, "200 次随机插入，每次插入后【根叶黑/不红红/黑路同/BST有序】全部成立");
  }
  CHECK(t.size() == 200, "size == 200");
  CHECK(t.height() <= 2.0 * std::log2(201.0),
        "高度不得超过理论上界 2*log2(n+1)（黑路同的直接推论）");
  std::printf("  [观察] 200 个随机数: 高度=%d, 黑高=%d, log2(200)=%.1f\n",
              t.height(), t.blackHeight(), std::log2(200.0));
}

// ---------------------------- 第3节 · 对抗性输入 ----------------------------
static void testAdversarial() {
  section("第3节 · 对抗性输入：朴素 BST 的死穴，红黑树必须稳如老狗");

  const int N = 1000;
  const double bound = 2.0 * std::log2(static_cast<double>(N) + 1.0);

  // 3.1 递增插入 —— 直接把朴素 BST 打成链表的输入
  {
    RedBlackTree<int> t;
    std::vector<int> keys(N);
    std::iota(keys.begin(), keys.end(), 0);
    if (insertAllAndValidate(t, keys))
      CHECK(true, "1000 个递增数逐个插入，全程性质合法");
    CHECK(t.height() <= bound, "递增插入后高度 <= 2*log2(n+1)");
    std::vector<int> v = t.inorder();  // 先存下来再检查（临时 vector 迭代器会悬空）
    CHECK(std::is_sorted(v.begin(), v.end()), "中序仍有序");
    std::printf("  [观察] 递增 1000 个: 高度=%d（朴素 BST 会是 1000）\n", t.height());
  }
  // 3.2 递减插入 —— 镜像死穴
  {
    RedBlackTree<int> t;
    std::vector<int> keys(N);
    std::iota(keys.begin(), keys.end(), 0);
    std::reverse(keys.begin(), keys.end());
    if (insertAllAndValidate(t, keys))
      CHECK(true, "1000 个递减数逐个插入，全程性质合法");
    CHECK(t.height() <= bound, "递减插入后高度 <= 2*log2(n+1)");
  }
  // 3.3 锯齿插入 —— 一小一大来回跳
  {
    RedBlackTree<int> t;
    std::vector<int> keys;
    keys.reserve(N);
    for (int i = 0; i < N / 2; ++i) {
      keys.push_back(i);
      keys.push_back(N - 1 - i);
    }
    if (insertAllAndValidate(t, keys))
      CHECK(true, "1000 个锯齿形数逐个插入，全程性质合法");
    CHECK(t.height() <= bound, "锯齿插入后高度 <= 2*log2(n+1)");
  }
}

// ---------------------------- 第4节 · 随机压测 ----------------------------
static void testRandomStress() {
  section("第4节 · 两万随机数压测");
  const int N = 20000;
  std::mt19937 rng(20260920);

  // 只插偶数；奇数天然是"必未命中"的查询
  std::vector<int> keys(N);
  for (int i = 0; i < N; ++i) keys[i] = i * 2;
  std::shuffle(keys.begin(), keys.end(), rng);

  RedBlackTree<int> t;
  for (int k : keys) t.insert(k);
  CHECK(t.size() == static_cast<std::size_t>(N), "size == 20000");

  bool allHit = true;
  for (int k : keys)
    if (!t.contains(k)) { allHit = false; break; }
  CHECK(allHit, "两万个已插入的数必须全部能查到");

  bool allMiss = true;
  for (int i = 0; i < 2000; ++i) {
    int odd = static_cast<int>(rng() % (2 * N)) | 1;  // 强制为奇数
    if (t.contains(odd)) { allMiss = false; break; }
  }
  CHECK(allMiss, "未插入的奇数一个都不应查到");

  std::string err;
  CHECK(t.validate(&err), std::string("大规模插入后全树校验: ") + err);
  std::vector<int> sorted = t.inorder();
  CHECK(std::is_sorted(sorted.begin(), sorted.end()),
        "两万个数的中序遍历完全有序");
  CHECK(t.height() <= 2.0 * std::log2(static_cast<double>(N) + 1.0),
        "高度 <= 2*log2(n+1)");
  std::printf("  [观察] n=20000: 高度=%d, 黑高=%d, log2(n)=%.1f, "
              "高度/log2(n)=%.2f\n",
              t.height(), t.blackHeight(), std::log2(static_cast<double>(N)),
              t.height() / std::log2(static_cast<double>(N)));
}

// ---------------------------- 第5节 · RB vs AVL ----------------------------
static void testVsAvl() {
  section("第5节 · 红黑树 vs AVL：优点与短板，用数据说话");
  const int N = 20000;
  std::mt19937 rng(20260920);

  // ---- 5.1 递增插入比旋转次数：红黑树的看家本领（断言）----
  {
    RedBlackTree<int> rb;
    AvlTree<int> avl;
    for (int i = 0; i < N; ++i) {
      rb.insert(i);
      avl.insert(i);
    }
    int rbR = rb.rotations(), avlR = avl.rotations();
    std::printf("  递增插入 %d 个数:  红黑树旋转 %d 次 | AVL 旋转 %d 次\n", N, rbR, avlR);
    CHECK(rbR <= avlR,
          "同样的插入序列，红黑树旋转次数不应多于 AVL（红黑树更'省'的核心证据）");
    CHECK(rbR > 0 && avlR > 0, "两者都应发生旋转");
    std::printf("  [结论] 插入场景红黑树调整更少 —— std::map / Linux 内核选它的原因\n");
  }

  // ---- 5.2 随机插入：旋转计数（观察项）+ 建两棵树供 5.3/5.4 使用 ----
  std::vector<int> keys(N);
  for (int i = 0; i < N; ++i) keys[i] = i * 2;
  std::shuffle(keys.begin(), keys.end(), rng);
  RedBlackTree<int> rb;
  AvlTree<int> avl;
  for (int k : keys) {
    rb.insert(k);
    avl.insert(k);
  }
  std::printf("  随机插入 %d 个数:  红黑树旋转 %d 次 | AVL 旋转 %d 次（观察项）\n",
              N, rb.rotations(), avl.rotations());

  // ---- 5.3 树高对比：AVL 更矮（查找略快的来源），RB 宽松但有 2log 上界 ----
  {
    double lg = std::log2(static_cast<double>(N) + 1.0);
    std::printf("  树高对比: log2(n)=%.1f | 红黑树 %d | AVL %d | 红黑树黑高 %d\n",
                lg, rb.height(), avl.height(), rb.blackHeight());
    CHECK(rb.height() <= 2.0 * lg,
          "红黑树高度上界 2*log2(n+1)（口诀【黑路同】换来的保证）");
    CHECK(avl.height() <= 1.4405 * lg + 1.0,
          "AVL 高度上界约 1.44*log2(n)");
    std::printf("  [结论] AVL 更矮 → 单次查找路径略短；红黑树稍高但仍在 2log 内"
                " → 换来更新时更少的旋转。这就是两者取舍的本质。\n");
  }

  // ---- 5.4 查找耗时对比（观察项，受机器/缓存影响，仅供参考）----
  {
    const int M = 2000000;
    std::vector<int> q(M);
    for (int i = 0; i < M; ++i) {
      // 用 size_t 做乘法避免 int 溢出；1/3 的查询故意落空
      int v = keys[(static_cast<std::size_t>(i) * 48271u) % keys.size()];
      q[static_cast<std::size_t>(i)] = (i % 3 == 0) ? v + 1 : v;
    }
    long long hit1 = 0, hit2 = 0;
    double msRb = benchMs([&] {
      long long acc = 0;
      for (int i = 0; i < M; ++i) acc += rb.contains(q[static_cast<std::size_t>(i)]);
      hit1 = acc;
    });
    double msAvl = benchMs([&] {
      long long acc = 0;
      for (int i = 0; i < M; ++i) acc += avl.contains(q[static_cast<std::size_t>(i)]);
      hit2 = acc;
    });
    CHECK(hit1 == hit2 && hit1 > 0, "两棵树的命中数应完全一致");
    std::printf("  查找 %d 次: 红黑树 %.1f ms (%.0f ns/次) | AVL %.1f ms (%.0f ns/次)"
                "（观察项）\n",
                M, msRb, msRb * 1e6 / M, msAvl, msAvl * 1e6 / M);
    std::printf("  [结论] 理论上 AVL 更矮查找略快；但都在 O(log n) 档位，差距远小于"
                "它们与朴素 BST 的差距。\n");
  }
}

// ---------------------------- 第6节 · RB vs 朴素 BST ----------------------------
static void testVsNaiveBst() {
  section("第6节 · 红黑树 vs 朴素 BST：它到底解决了什么");
  const int N = 5000;

  NaiveBst<int> naive;
  RedBlackTree<int> rb;
  for (int i = 0; i < N; ++i) {
    naive.insert(i);
    rb.insert(i);
  }
  std::printf("  同样递增插入 %d 个数: 朴素 BST 高度 %d | 红黑树高度 %d\n",
              N, naive.height(), rb.height());
  CHECK(naive.height() == N, "朴素 BST 递增插入应退化成链表（高度 = n）");
  CHECK(rb.height() <= 2.0 * std::log2(static_cast<double>(N) + 1.0),
        "红黑树高度仍被压在 2*log2(n+1) 内");

  // 查几把感受一下差距（观察项：链表上查找要走 O(n) 步）
  const int M = 2000;
  long long hit1 = 0, hit2 = 0;
  double msNaive = benchMs([&] {
    long long acc = 0;
    for (int i = 0; i < M; ++i) acc += naive.contains(i * 7 % N);
    hit1 = acc;
  });
  double msRb = benchMs([&] {
    long long acc = 0;
    for (int i = 0; i < M; ++i) acc += rb.contains(i * 7 % N);
    hit2 = acc;
  });
  CHECK(hit1 == hit2 && hit1 > 0, "两棵树命中数应一致");
  std::printf("  各查 %d 次: 朴素 BST %.2f ms | 红黑树 %.2f ms（观察项）\n",
              M, msNaive, msRb);
  std::printf("  [结论] 没有红黑树的平衡保证，二叉搜索树在最坏输入下名存实亡。\n");
}

// ---------------------------- 第7节 · 删除（进阶） ----------------------------
static void testErase() {
  section("第7节 · 删除 erase（进阶 TODO⑥ / 参考答案）");
  const int N = 3000;
  std::mt19937 rng(7);
  std::vector<int> keys(N);
  for (int i = 0; i < N; ++i) keys[i] = i * 2;
  std::shuffle(keys.begin(), keys.end(), rng);

  RedBlackTree<int> t;
  for (int k : keys) t.insert(k);

  // 删掉一半，过程中周期性校验性质（删除比插入更容易破坏黑路同）
  bool eraseOk = true;
  for (int i = 0; i < N / 2; ++i) {
    if (!t.erase(keys[static_cast<std::size_t>(i)])) eraseOk = false;
    if (i % 200 == 199) {
      std::string err;
      if (!t.validate(&err)) {
        CHECK(false, "删除 " + std::to_string(i + 1) + " 个后性质校验失败: " + err);
        eraseOk = false;
        break;
      }
    }
  }
  CHECK(eraseOk, "删除 1500 个数，全部成功且过程性质合法");

  bool gone = true, remain = true;
  for (int i = 0; i < N / 2; ++i)
    if (t.contains(keys[static_cast<std::size_t>(i)])) { gone = false; break; }
  for (int i = N / 2; i < N; ++i)
    if (!t.contains(keys[static_cast<std::size_t>(i)])) { remain = false; break; }
  CHECK(gone, "被删的键不应再查到");
  CHECK(remain, "留下的键必须都能查到");
  CHECK(t.size() == static_cast<std::size_t>(N / 2), "size 应剩一半");
  CHECK(!t.erase(999999), "删除不存在的键应返回 false");

  // 一路删空，再插回来
  for (int i = N / 2; i < N; ++i) t.erase(keys[static_cast<std::size_t>(i)]);
  std::string err;
  CHECK(t.empty() && t.validate(&err), std::string("删空后应为空树且性质合法: ") + err);
  for (int i = 0; i < 100; ++i) t.insert(i);
  CHECK(t.validate(&err) && t.size() == 100, std::string("重新插入 100 个后校验: ") + err);
}

// ---------------------------- 主流程 ----------------------------
int main() {
  std::printf("红黑树测试套件 —— 当前测试对象: %s\n", TEST_TARGET);
  std::printf("（想验证参考答案全绿: ./run_tests.sh answer）\n");

  bool pIns = false, pCon = false, pRot = false, pErase = false;
  section("第0节 · 实现探测（自动判断练习进度）");
  pIns = probeInsert();
  if (pIns) {
    pCon = probeContains();
  } else {
    skipNote("contains 探测（依赖 insert）");
  }
  pRot = probeRotateAndFixup();
  if (pIns && pCon) {
    pErase = probeErase();
  } else {
    skipNote("erase 探测（依赖 insert/contains）");
  }

  const bool coreReady = pIns && pCon && pRot;
  if (!coreReady) {
    std::printf("\n  +------------------------------------------------------------+\n");
    std::printf("  |  核心部分尚未完成 —— 这就是你的起点，完全正常。             |\n");
    std::printf("  |  打开 practice/red_black_tree.h，按顺序补全：              |\n");
    std::printf("  |    TODO① contains -> ②③ 左右旋 -> ④ insert -> ⑤ fixup    |\n");
    std::printf("  |  然后回来再跑一次 ./run_tests.sh                           |\n");
    std::printf("  |  卡住了就看 answer/red_black_tree_answer.h 对应函数。      |\n");
    std::printf("  +------------------------------------------------------------+\n");
  }

  if (coreReady) {
    testBasics();
    testMottoEveryInsert();
    testAdversarial();
    testRandomStress();
    testVsAvl();
    testVsNaiveBst();
  } else {
    section("第1~6节");
    skipNote("核心功能未完成 —— 完成练习版 TODO 后自动开放");
  }

  if (coreReady && pErase) {
    testErase();
  } else {
    section("第7节 · 删除（进阶）");
    skipNote(pErase ? "依赖核心功能" : "erase 未实现 —— 完成 TODO⑥ 后自动开放");
  }

  std::printf("\n========== 总结 ==========\n");
  std::printf("  通过 %d | 失败 %d | 跳过 %d\n", gPass, gFail, gSkip);
  if (gFail == 0 && coreReady) {
    std::printf("  全部通过！口诀三连（根叶黑/不红红/黑路同）在你的实现里站住了脚。\n");
  } else if (gFail == 0) {
    std::printf("  没有失败项 —— 去完成 practice/ 里的 TODO 吧。\n");
  } else {
    std::printf("  有失败项：按上面的 [FAIL] 行号定位；\n"
                "  调试建议: validate(&err) 会说违反了哪条口诀，t.draw() 可以看树形。\n");
  }
  return gFail == 0 ? 0 : 1;
}
