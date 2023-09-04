# ImDui
[中文说明](https://github.com/Ray1024/ImDui/blob/master/README-cn.md)

ImDui is a lightweight immediate mode graphical user interface based on Direct2D for c++. 
It also support GPU Accelerated.
The immediate mode graphical user interface is well suited for real-time rendered applications.
ImDui can be used for many scenes, such as game engine editors and small tiny applications.

## Usage
You can see the userguide in the file [main.cpp](https://github.com/Ray1024/ImDui/blob/master/ImDui/main.cpp).
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

## Screenshots
![sample1](https://github.com/Ray1024/ImDui/blob/master/samples/sample1.png)

![sample2](https://github.com/Ray1024/ImDui/blob/master/samples/sample2.png)

## Demo
the demo application here: [ImDui_test](https://github.com/Ray1024/ImDui/tree/master/samples/ImDui_test)

## 公众号
关注公众号:江湖码客Mark，有任何问题都可解答。</br>
![公众号](https://github.com/Ray1024/Direct2D/blob/master/images/qrcode_for_gh_16bcaf1d516e_258.jpg)