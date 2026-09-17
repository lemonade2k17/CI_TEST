# ci-demo：一个能跑通 CI 的最小嵌入式风格项目

这个项目的唯一目的：**让你亲眼看到 CI 是怎么跑起来的。**

它模拟了一个真实的嵌入式场景——一个 ADC 滑动平均滤波器。
但注意 `src/adc_filter.c` 里**没有任何硬件相关代码**，所以它能在 PC 上编译和测试，
也就能在 GitHub 的服务器上自动跑。

这条链就是全部的秘密：

```
代码解耦（不依赖硬件） → 能在 PC 上测 → 能挂到 CI 上自动跑
```

---

## 一、先在本地跑起来

需要 CMake 和一个 C 编译器（MinGW-w64 / MSVC / gcc 都行）。

```powershell
cd ci-demo
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

你会看到 7 组测试、共 15 项检查全部通过。也可以直接运行看详细输出：

```powershell
.\build\test_adc_filter.exe
```

### 如果本地没有 CMake / 编译器

**这非常常见，而且恰好说明了 CI 的价值。** 不用为了跑个测试去折腾本地环境，
直接跳到第二节推到 GitHub，让服务器替你跑。

如果就是想在本机验证，用一条 gcc 命令绕过 CMake 即可：

```powershell
gcc -std=c11 -Wall -Wextra -I include src/adc_filter.c tests/test_adc_filter.c -o test.exe
.\test.exe
```

要正经搭本地环境的话，Windows 上推荐装 **MSYS2**（自带 gcc + cmake + make 全套），
它也是嵌入式圈在 Windows 上最常用的工具链环境。

---

## 二、把它推上 GitHub，看 CI 真的跑起来

### 1. 在 GitHub 上建一个空仓库

打开 https://github.com/new ，填个名字（比如 `ci-demo`），
**不要**勾选 "Add a README file"，直接 Create。

### 2. 把本地代码推上去

把下面 URL 换成你自己仓库的地址：

```powershell
cd ci-demo
git init
git add .
git commit -m "feat: ADC 滑动平均滤波器 + 单元测试 + CI"
git branch -M main
git remote add origin https://github.com/<你的用户名>/ci-demo.git
git push -u origin main
```

### 3. 去看 CI 跑

推送完成后：

1. 打开你的仓库页面
2. 点顶部的 **Actions** 标签
3. 你会看到一个叫 **CI** 的工作流正在转圈（黄色）
4. 点进去，展开 `build-and-test`，能看到四个步骤逐条执行
5. 大约 30 秒后变成 **绿色的对勾** ✓

**这就是 CI。** 它跑在 GitHub 的服务器上，不是你自己的电脑。

---

## 三、关键一步：故意让它失败

这是最有价值的一步——**只有看到红的，你才真正理解 CI 的价值。**

打开 `tests/test_adc_filter.c`，把这一行：

```c
CHECK(adc_filter_update(&f, 3) == 2, "窗口满时均值为 6/3");
```

改成：

```c
CHECK(adc_filter_update(&f, 3) == 999, "故意写错的断言");
```

然后：

```powershell
git add .
git commit -m "test: 故意让测试失败"
git push
```

回到 Actions 页面，你会看到：

- 构建变成 **红色的叉** ✗
- 点进去，第 4 步 "运行单元测试" 报错，明确告诉你哪一行断言失败了
- 邮箱可能还会收到 GitHub 的失败通知

**这就是 CI 的反馈回路。** 在你把代码合并进主干之前，问题就被拦住了。

改回来，再 push 一次，它就变绿了。

---

## 四、再进一步：走一遍 Pull Request

单人在 main 上推太简单了，真实团队是这样工作的：

```powershell
git switch -c feature/median-filter      # 1. 开分支
# ... 改点代码 ...
git add .
git commit -m "feat: 增加中值滤波去除尖峰"
git push -u origin feature/median-filter # 2. 推到远端
```

推完后 GitHub 页面顶部会出现一个黄色横幅，写着 **"Compare & pull request"**：

3. 点它 → 填标题和描述 → 点 **Create pull request**
4. 这个 PR 页面上会自动挂上 CI 的运行状态
5. 等 CI 变绿，点 **Merge pull request**
6. 删掉分支，回到 main，`git pull` 拉下最新代码

**你现在就走完了一个完整的、工业界标准的开发流程。**

---

## 五、然后呢？迁移到你自己的项目

在真实嵌入式项目里，把上面的骨架复制过去：

| 这个 demo | 你的项目 |
|---|---|
| `src/adc_filter.c` | 你的协议解析 / 状态机 / 控制算法 |
| `include/adc_filter.h` | 对应的接口头文件 |
| `tests/test_adc_filter.c` | 换成 GoogleTest / Unity 也行 |
| `CMakeLists.txt` | 加上交叉编译的固件 target |
| `.github/workflows/ci.yml` | 加一步 `arm-none-eabi-gcc` 交叉编译 |

**唯一的硬性要求**：那份逻辑必须不依赖寄存器、不依赖 HAL、不依赖中断。
做不到就先重构——重构的过程本身就是最好的软件工程训练。

---

## 六、文件说明

```
ci-demo/
├── .github/workflows/ci.yml   ← CI 配置：告诉服务器跑什么命令
├── CMakeLists.txt             ← 构建脚本
├── include/adc_filter.h       ← 接口（纯逻辑，无硬件依赖）
├── src/adc_filter.c           ← 实现
├── tests/test_adc_filter.c    ← 单元测试（PC 上跑）
└── README.md                  ← 你正在读的这个
```

## 七、参考资料

- GitHub Actions 官方入门：https://docs.github.com/actions
- CMake 官方教程：https://cmake.org/cmake/help/latest/guide/tutorial/
- GoogleTest：https://google.github.io/googletest/
- Unity（嵌入式专用测试框架）：https://github.com/ThrowTheSwitch/Unity
