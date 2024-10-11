/* TinyWM 由 Nick Welch <nick@incise.org> 编写于2005年和2011年。
 *
 * 该软件是公共领域的软件，按“原样”提供，不提供任何担保。*/

/* tinywm 的主要目的是作为一个如何进行 X 操作和/或理解窗口管理器的
 * 非常基础的示例，因此我想在代码中加上解释的注释，但我真的不喜欢
 * 阅读注释过多的代码——更何况，tinywm 应该尽可能简洁，所以加很多
 * 注释并不合适。我希望 tinywm.c 是那种你看一眼就能感叹“哇，就这么
 * 简单？太酷了！”的程序。所以我把它复制到了 annotated.c 并加上了
 * 大量注释。啊，但是现在每次修改代码都要做两次！算了，我可以用一些
 * 脚本来去掉注释并写入 tinywm.c……还是算了吧。
 */

/* 大部分 X 的功能都包含在 Xlib.h 中，但有一些功能需要其他头文件，
 * 比如 Xmd.h、keysym.h 等等。
 */
#include <X11/Xlib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

int main(void)
{
    Display * dpy;
    XWindowAttributes attr;

    /* 我们用它来保存移动/调整大小开始时的鼠标指针状态。*/
    XButtonEvent start;

    XEvent ev;

    /* 如果无法连接，则返回失败状态 */
    if(!(dpy = XOpenDisplay(0x0))) return 1;

    /* 我们使用 DefaultRootWindow 获取根窗口，这是一种比较简单的做法，
     * 只适用于默认屏幕。大多数人只有一个屏幕，但不是所有人。如果你
     * 没有使用 xinerama 运行多头显示，你可能有多个屏幕。（我不确定
     * 供应商的特定实现，比如 nvidia 的）
     *
     * 许多，可能大多数窗口管理器只处理一个屏幕，所以实际上这并不
     * 算太天真。
     *
     * 如果你想获取特定屏幕的根窗口，可以使用 RootWindow()，但用户也
     * 可以控制哪个屏幕是我们的默认屏幕：如果他们将 $DISPLAY 设置为
     * “:0.foo”，那么我们的默认屏幕号就是他们指定的“foo”。
     */

    /* 你也可以包含 keysym.h 并使用 XK_F1 常量，而不是调用 XStringToKeysym，
     * 但这种方法更“动态”。想象一下，你有配置文件来指定按键绑定。与其
     * 解析按键名并有一个庞大的表来映射字符串到 XK_* 常量，不如直接把
     * 用户指定的字符串交给 XStringToKeysym。XStringToKeysym 会返回适当
     * 的 keysym，或者告诉你它是无效的键名。
     *
     * keysym 基本上是键的跨平台数字表示，如“F1”、“a”、“b”、“L”、
     * “5”、“Shift”等等。keycode 是键盘发送给 X 的由键盘驱动（或类似
     * 的东西——我不是硬件/驱动专家）产生的键的数字表示。所以我们绝不
     * 想硬编码 keycode，因为它们在不同系统之间可能会不同。
     */
    XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("F1")), Mod1Mask,
            DefaultRootWindow(dpy), True, GrabModeAsync, GrabModeAsync);

    /* XGrabKey 和 XGrabButton 基本上是说“当这种修饰键和按键/按钮的组合
     * 被按下时，把事件发送给我”。所以我们可以安全地假设我们会收到
     * Alt+F1 事件、Alt+Button1 事件和 Alt+Button3 事件，但不会收到其他事件。
     * 你可以像这样为键/鼠标组合单独抓取事件，也可以使用 XSelectInput
     * 配合 KeyPressMask/ButtonPressMask 等来捕捉所有这些类型的事件，然后
     * 在接收到它们时进行过滤。
     */
    XGrabButton(dpy, 1, Mod1Mask, DefaultRootWindow(dpy), True,
            ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(dpy, 3, Mod1Mask, DefaultRootWindow(dpy), True,
            ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);

    start.subwindow = None;
    for(;;)
    {
        /* 这是处理 X 事件的最基本方式；你可以通过使用 XPending()，或结合
         * ConnectionNumber() 和 select()（或 poll() 或其他你喜欢的方式）
         * 来让它更灵活。
         */
        XNextEvent(dpy, &ev);

        /* 这是我们用来提升窗口的按键绑定。正如我在 ratpoison 的 wiki 上
         * 看到有人提到的，这确实有点蠢；不过我想在这里加上某种键盘绑定，
         * 这是最合适的地方。
         *
         * 我对 .window 和 .subwindow 有点困惑了一段时间，但稍微阅读了一下
         * 文档就搞清楚了。我们的被动抓取是在根窗口上进行的，所以由于我们
         * 只关心它的子窗口的事件，我们查看 .subwindow。当 subwindow == None，
         * 这意味着事件发生的窗口和抓取的窗口是同一个窗口——在这种情况下，
         * 就是根窗口。
         */
        if(ev.type == KeyPress && ev.xkey.subwindow != None)
            XRaiseWindow(dpy, ev.xkey.subwindow);
        else if(ev.type == ButtonPress && ev.xbutton.subwindow != None)
        {
            /* 我们“记住”了鼠标指针在移动/调整大小开始时的位置，以及窗口的
             * 大小/位置。这样，当鼠标指针移动时，我们可以将其与初始数据进行
             * 比较，并相应地移动/调整大小。
             */
            XGetWindowAttributes(dpy, ev.xbutton.subwindow, &attr);
            start = ev.xbutton;
        }
        /* 我们只在按下按钮时接收到鼠标移动事件，但我们仍然需要检查拖动
         * 是否在窗口上开始。
         */
        else if(ev.type == MotionNotify && start.subwindow != None)
        {
            /* 在这里我们可以通过以下方式“压缩”鼠标移动事件：
             *
             * while(XCheckTypedEvent(dpy, MotionNotify, &ev));
             *
             * 如果有 10 个事件在等待，除了最最近的事件外，其他的都没有
             * 意义。在某些情况下——比如窗口非常大或系统运行缓慢时——
             * 如果不这样做，可能会导致大量的“拖动延迟”，尤其是当你的窗口
             * 管理器进行很多绘图等操作时，这会导致它延迟。
             *
             * 对于带有桌面切换功能的窗口管理器，压缩 EnterNotify 事件
             * 也很有用，这样可以避免当窗口在指针下移动时出现“焦点闪烁”。
             */

            /* 现在我们使用在移动/调整大小开始时保存的数据，并将其与鼠标
             * 指针的当前位置进行比较，以确定窗口的新大小或位置。
             *
             * 如果最初按下的是鼠标左键，则我们移动窗口。否则按下的是鼠标
             * 右键，我们调整窗口大小。
             *
             * 我们还确保窗口的尺寸不为负数，否则会出现“包裹”现象，使得窗口
             * 宽度变成 65000 像素左右（通常伴随着大量的交换和速度变慢）。
             *
             * 更糟糕的是，如果我们“幸运地”使宽度或高度恰好为零，X 将会产生
             * 一个错误。所以我们指定最小的宽度/高度为 1 像素。
             */
            int xdiff = ev.xbutton.x_root - start.x_root;
            int ydiff = ev.xbutton.y_root - start.y_root;
            XMoveResizeWindow(dpy, start.subwindow,
                attr.x + (start.button==1 ? xdiff : 0),
                attr.y + (start.button==1 ? ydiff : 0),
                MAX(1, attr.width + (start.button==3 ? xdiff : 0)),
                MAX(1, attr.height + (start.button==3 ? ydiff : 0)));
        }
        else if(ev.type == ButtonRelease)
        {
            start.subwindow = None;
        }
    }
}
