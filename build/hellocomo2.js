import * as como from "/home/xilongpei/como/como/out/target/como.linux.x64.rls/samples/democomponent/component/FooBarDemo.so";

for (let k in como) {
    console.log(k, '=', como[k].toString());
}
