# como_quickjs

#### 介绍
como_quickjs是COMO QuickJS 绑定库。

QuickJS 是在 MIT 许可下发的一个轻量 js 引擎包含 js 的编译器和解释器，支持最新 TC39 的
ECMA-262 标准。QuickJS的作者 Fabrice Bellard 是一个传奇人物，速度最快的TCC，还有功能
强大，使用最广的视频处理库 FFmpeg 和 Android 模拟器 QEMU 都是出自他手。
QuickJS通过反射机制调用 COMO 构件，不需要 COMO 做任何改变，写出的构件就可以
在 Javascript 中使用。只要是用COMO规范开发的C++程序，就可以象 Javascript 自己写的程序
模块 module 一样，但效率高得多。

#### 安装教程

首先进 COMO 工作环境：
```shell
- como_linux_x64:       Switch to build como for linux x64.
- como_linux_aarch64:   Switch to build como for linux aarch64.
- como_linux_riscv64:   Switch to build como for linux riscv64.
- como_android_aarch64: Switch to build como for android aarch64.
- como_openEuler_riscv: Switch to build como for openEuler RISC-V.
```


#### 使用说明
```js
/* example of JS module importing a COMO module */
import { Point } from "./point.so";

function assert(b, str)
{
    if (b) {
        return;
    } else {
        throw Error("assertion failed: " + str);
    }
}

class ColorPoint extends Point {
    constructor(x, y, color) {
        super(x, y);
        this.color = color;
    }
    get_color() {
        return this.color;
    }
};

function main()
{
    var pt, pt2;

    pt = new Point(2, 3);
    assert(pt.x === 2);
    assert(pt.y === 3);
    pt.x = 4;
    assert(pt.x === 4);
    assert(pt.norm() == 5);

    pt2 = new ColorPoint(2, 3, 0xffffff);
    assert(pt2.x === 2);
    assert(pt2.color === 0xffffff);
    assert(pt2.get_color() === 0xffffff);
}

```