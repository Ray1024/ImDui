# ImDui
ImDui是一个C++实现的基于Direct2D渲染的轻量级即时渲染GUI框架。它还支持GPU硬件加速。
即时渲染类型的GUI框架适合于实时渲染的应用程序。ImDui可以应用于很多场景，比如游戏引擎编辑器和小型的程序等等。

## 使用说明
你可以在[main.cpp](https://github.com/Ray1024/ImDui/blob/master/ImDui/main.cpp)文件中查看使用说明.
```
while()
{
    ...
    ImDui::NewFrame();

    // test codes

	ImDui::BeginWindow("ImDui Demo", &show_demo, ImFloat2(20, 20), ImFloat2(400, 200));
	ImDui::Text("Hello ImDui!");
	ImDui::Button("I am a button.");

	static float val_slider = 0.5;
	ImDui::SliderFloat("slider", &val_slider, 0.0f, 1.0f);

	static bool val1 = false;
	static bool val2 = false;
	ImDui::CheckBox("checkbox1", &val1); ImDui::SameLine(100);
	ImDui::CheckBox("checkbox2", &val2);

	static int e = 0;
	ImDui::RadioButton("radio a", &e, 0); ImDui::SameLine(100);
	ImDui::RadioButton("radio b", &e, 1); ImDui::SameLine(200);
	ImDui::RadioButton("radio c", &e, 2);

	static float col1[3] = { 1.0f,0.0f,0.2f };
	ImDui::ColorEdit3("color editor 1", col1);
	ImDui::EndWindow();

    g_pMainRT->BeginDraw();
    g_pMainRT->Clear(clear_color.ToD2DColorF());
    ImDui::Render();
    g_pMainRT->EndDraw();
    ...
}
```

## 截图
![sample1](https://github.com/Ray1024/ImDui/blob/master/samples/sample1.png)

![sample2](https://github.com/Ray1024/ImDui/blob/master/samples/sample2.png)

## 示例
示例程序: [ImDui_test](https://github.com/Ray1024/ImDui/tree/master/samples/ImDui_test)