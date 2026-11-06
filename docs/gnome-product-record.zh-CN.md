# GNOME 产品记录:PinZ(帧贴)

日期:2026-11-06  
状态:工作记录;PinZ 是当前产品名,尚未执行全仓重命名

## 1. 产品定位

面向 GNOME Wayland 的性能优先截图与贴图工具,目标是提供接近 PixPin 的核心体验:

1. 按快捷键后快速冻结屏幕并框选区域。
2. 原地标注、复制、保存、OCR、搜索、上传或录制。
3. 把截图作为始终置顶的可移动贴图继续参考。
4. 在贴图上继续 OCR 和图像处理。

当前代码基于 EShot,但 GNOME 分支的产品方向已经不是普通截图工具,而是“低延迟截图 + 贴图工作台”。

## 2. 建议名称

推荐:**PinZ**,中文工作名 **帧贴**。

| 候选名 | 结论 | 原因 |
| --- | --- | --- |
| **PinZ / 帧贴** | 当前选择 | `Pin` 的变形,短、快、有品牌感;`Z` 结尾带来速度感。本轮未发现同名截图工具;需用副标题说明它是 screenshot/pin 工具 |
| Pinux | 备选,不优先 | 能体现 Linux 生态,但已有历史 PINUX 开源操作系统项目,且容易被误解为 Linux 发行版;没有直接表达截图或贴图 |
| QuickPin | 备选 | 一眼能看出“快速 Pin”,但音节比 PinZ 长,且已有同名笔记、Pinboard、设计反馈产品 |
| CapturePin | 已撤回 | 语义准确,但 `Capture` 音节较长,不如 PinZ 轻 |
| PinShot | 不推荐 | 已有多个同名或近似产品,其中包括截图、标注、置顶工具;同品类冲突太直接 |
| PinScreen | 不推荐 | `Pinscreen` 是已存在的视觉 AI 公司/品牌;虽然没有发现同名 GNOME 截图工具,但精确品牌名冲突风险高 |
| PinFrame | 已撤回 | 虽未检索到明确截图产品,但用户希望避开与 FramePin 的倒置相似 |
| FramePin | 不推荐 | 已有 framepin.com 产品,名称冲突风险高 |
| ScreenPin | 不推荐 | 已有 macOS 截图/贴图工具和屏幕 overlay 项目 |
| PinSnap | 不推荐 | 已有 iOS 拍摄/标记应用和 Pinterest 图片下载扩展 |
| PinPane | 不推荐 | 已有 Windows 截图置顶工具 |
| ShotPin | 不推荐 | 已有被标记并下架的同名浏览器扩展,搜索印象差 |
| ShotDock | 不推荐 | 已有截图浏览器扩展和 Wayland 截图相关项目 |
| Pinpoint | 不推荐 | GNOME 历史上已有 Pinpoint 应用,生态内混淆 |
| SnapPin | 备选 | 易读,但 Snap 容易让 Linux 用户误解为 Snap 包,且不能体现帧缓存 |
| PixPin for GNOME | 禁止作为正式名 | PixPin 是闭源第三方产品,存在商标和来源混淆风险 |

正式发布前仍需做商标、域名、Flathub/App ID 和发行版包名检查。

品牌图标源文件:[images/pinZ-icon.svg](images/pinZ-icon.svg)

![PinZ icon](images/pinZ-icon.svg)

图标语义:深色圆角底代表被冻结的屏幕帧,电光黄 Z 型闪电对应名称中的 `Z`,青色外轮廓是地图 Pin,表示截图可以立即固定在桌面上。

## 3. 核心设计原则

### 3.1 把等待从用户操作路径移走

原路径:

```text
按截图键
→ Screenshot portal 请求
→ GNOME 合成器截图
→ 返回完整图片
→ 创建/显示 overlay
→ 等待用户输入
```

当前路径:

```text
屏幕变化
→ 用户授权一次的持久 ScreenCast 流
→ PipeWire/GStreamer 后台接收
→ 只保留最新帧

按截图键
→ 直接消费最新帧
→ 显示 overlay
→ 接收选区输入
```

权限仍在 GNOME portal 中,不是绕过安全边界;被移走的是用户等待。

### 3.2 常驻流必须有资源上限

当前策略:

- 默认关闭,用户显式开启 Fast capture。
- 首次由 GNOME ScreenCast picker 授权;后续使用 portal restore token。
- GNOME 屏幕共享指示保持可见,用户可撤销。
- GStreamer 限制为 10 FPS。
- queue 最多 2 帧,旧帧直接丢弃。
- 只保留最新帧,不做历史缓存。
- 帧超过新鲜度阈值、流停止、非 GNOME、非 Wayland 或多显示器时回退一次性 Screenshot portal。

### 3.3 不用内存浪费换表面速度

已做:PipeWire/GStreamer buffer 到 `QImage` 的只读共享映射,后台不再每 100 ms 复制一整张 4K BGRA 帧。

暂不做:提前把每一帧都转成 `QPixmap`。多数后台帧永远不会被用户截取;持续转换 4K 帧会浪费内存带宽和显存。

后续候选:DMA-BUF/GPU texture 零拷贝。该方向需要把 overlay 渲染从 Qt Widgets/QPixmap 迁向 QRhi/OpenGL/Vulkan,是技术栈级重构,必须先用 profiling 证明收益。

## 4. 当前已实现成果

### 4.1 GNOME Fast Capture

实现文件:

- `src/core/LinuxFastCapture.h`
- `src/core/LinuxFastCapture.cpp`
- `src/core/LinuxPortalScreenCast.h`
- `src/core/LinuxPortalScreenCast.cpp`

当前管道:

```text
pipewiresrc
→ videorate(max 10 FPS)
→ queue(leaky, max 2 buffers)
→ videoconvert
→ appsink(BGRA)
→ 只读 QImage 映射最新帧
```

`QImage` 通过 cleanup 回调持有 GStreamer buffer 生命周期;最后一个 `QImage` 引用销毁时才执行:

```cpp
gst_video_frame_unmap()
gst_buffer_unref()
```

### 4.2 截图 overlay 接入最新帧

`CaptureOverlay` 在 GNOME 上优先使用 `LinuxFastCapture::latestFrame()`。失败时自动使用原来的一次性 `Screenshot` portal,不需要用户干预。

### 4.3 XWayland 输入路径修复

GNOME Wayland 下 Qt overlay 通过 XWayland 显示。当前 override-redirect overlay 直接抓取键盘,不再依赖每次截图弹出的隐藏 focus proxy。隐藏 proxy 只保留给真实文本编辑场景。

同时,portal App ID 注册移动到 `QApplication` 构造之前,避免 Qt QPA 先用空 App ID 访问 portal。

### 4.4 贴图 OCR

贴图窗口右键菜单新增本地 OCR,补足 PixPin 风格“截图贴住后继续取文字”的工作流。

## 5. 实测性能

环境:Ubuntu GNOME Wayland,Qt 走 XWayland,单显示器,PipeWire 帧为 3840×2160 BGRA。

| 阶段 | 常驻流首次实现 | 共享帧优化后 |
| --- | ---: | ---: |
| 窗口呈现返回 | 8 ms | 10 ms |
| 首帧绘制 | 19 ms | 17 ms |
| 指针移动进入应用 | 34 ms | 34 ms |
| 鼠标按下进入应用 | 181 ms | 150 ms |
| 选区完成 | 280 ms | 250 ms |
| 常驻流 CPU 占用 | 未测 | 0.2% / 5 秒 |

说明:鼠标按下和选区完成包含内核虚拟输入测试中的固定等待;overlay 自身首帧绘制已达到约 17 ms,接近一帧时间。

回归验证:

- 构建通过。
- Fast Capture 测试 9/9 通过。
- 全量测试 69/69 通过。
- 真实 GNOME 桌面完成选区并读取 PipeWire 帧。
- 临时 `/dev/uinput` 测试 ACL 已撤销。
- 测试用 ydotool daemon 和 tracer 已停止。

## 6. 当前限制

1. Fast Capture 目前只支持 **GNOME Wayland 单显示器**;多显示器自动回退。
2. Qt Widgets 路径仍有截图时的 `QImage → QPixmap` 转换。
3. XWayland overlay 仍是兼容性方案,不是原生 Wayland surface。
4. GNOME GlobalShortcuts portal 版本会影响快捷键体验。
5. 长截图/滚动截图尚未实现。
6. PinZ 是当前产品名,但尚未改应用 ID、desktop 文件、二进制名和用户可见标题。

## 7. 下一步优先级

### P0:稳定性

- 长时间运行观察 PipeWire 会话是否被 GNOME 暂停或撤销。
- 锁屏、解锁、休眠、显示器热插拔后的恢复行为。
- Fast Capture 开启/关闭切换和 portal token 失效后的回退。

### P1:低延迟完善

- 缓存屏幕几何信息,在屏幕变化信号时失效。
- 异步获取截图前活动窗口标题,移出截图开始路径。
- 检查 `QSettings` 和 toolbar 配置读取是否仍可提前。
- 多显示器 Fast Capture 需要逐输出流和坐标映射,不能直接拼接单帧。

### P2:技术栈级优化

只有在 profiling 证明 `QImage → QPixmap` 或 `videoconvert` 成为主要瓶颈后才考虑:

- QRhi/OpenGL/Vulkan overlay。
- DMA-BUF 导入。
- GPU texture 直接显示。
- Rust 或原生 Wayland 重写不是当前必要步骤。

## 8. 产品判断

Qt 不是当前主要瓶颈。当前真正的性能来源是:

- portal 权限提前授权;
- PipeWire 帧提前到达;
- 后台只保留最新帧;
- 帧内存不重复复制;
- overlay 输入路径不绕隐藏窗口。

因此短期继续沿 C++/Qt6 路线优化,比重写技术栈更符合“性能优先但不浪费资源”的目标。
