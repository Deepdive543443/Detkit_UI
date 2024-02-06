#include "sections.h"

#define CANVAS_WIDTH 80
#define CANVAS_HEIGHT 80

extern lv_group_t *g;
extern lv_obj_t *box_list; 
extern Detector det;
extern BoxVec objects; 
extern bool detecting;
extern void *cam_buf;

extern lv_obj_t *btn_A;
extern lv_obj_t *btn_B;
extern lv_obj_t *btn_X;
extern lv_obj_t *btn_Y;
extern lv_indev_t *indev_A;
extern lv_indev_t *indev_B;
extern lv_indev_t *indev_X;
extern lv_indev_t *indev_Y;
extern lv_point_t point_array_A[2];
extern lv_point_t point_array_B[2];
extern lv_point_t point_array_X[2];
extern lv_point_t point_array_Y[2]; //Encoder 

static lv_style_t btn_btm;
static lv_style_t btn_press;

static void button_style_init(lv_style_t *btn)
{
    lv_style_init(btn);
    lv_style_set_border_width(btn, 1);
    lv_style_set_border_color(btn, lv_color_hex(0x999999));
    lv_style_set_radius(btn, 0);
    lv_style_set_pad_all(btn, 0);
    lv_style_set_bg_opa(btn, LV_OPA_COVER);
    lv_style_set_bg_grad_color(btn, lv_palette_main(LV_PALETTE_GREY));
    lv_style_set_bg_grad_dir(btn, LV_GRAD_DIR_VER);
}


void object_display_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *item = lv_event_get_target(e);

    switch(code) 
    {
        case LV_EVENT_CLICKED:

            unsigned char *pixels = malloc(sizeof(unsigned char) * WIDTH_TOP * HEIGHT_TOP * 3);
            writeCamToPixels(pixels, cam_buf, 0, 0, WIDTH_TOP, HEIGHT_TOP);

            int idx = lv_obj_get_child_id(item) - 2;
            if (idx < 0)
            {
                draw_boxxes(pixels, WIDTH_TOP, HEIGHT_TOP, &objects);
            }
            else
            {
                BoxInfo obj = BoxVec_getItem(idx, &objects);//objects.getItem(idx);

                BoxVec box_Vec_temp;
                create_box_vector(&box_Vec_temp, 1);
                BoxVec_push_back(obj, &box_Vec_temp);
                draw_boxxes(pixels, WIDTH_TOP, HEIGHT_TOP, &box_Vec_temp);
                BoxVec_free(&box_Vec_temp);
            }


            writePixelsToFrameBuffer(
                gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL), 
                pixels, 
                0, 
                0, 
                WIDTH_TOP, 
                HEIGHT_TOP
            );

            gfxFlushBuffers();
            gfxScreenSwapBuffers(GFX_TOP, true);
            gspWaitForVBlank();

            free(pixels);    
            break;

        default:
            break;

    }   
}

void display_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    switch(code) 
    {
        case LV_EVENT_PRESSED:
            // Hang up the video until inference finished
            if(detecting)
            {
                lv_obj_del(box_list); 
            }
            detecting = true;
            lv_obj_del_async(btn_A);
            BoxVec_free(&objects);
            pause_cam_capture(cam_buf);

            // Inference 
            unsigned char *pixels = malloc(sizeof(unsigned char) * WIDTH_TOP * HEIGHT_TOP * 3);
            writeCamToPixels(pixels, cam_buf, 0, 0, WIDTH_TOP, HEIGHT_TOP);
            objects = det.detect(pixels, WIDTH_TOP, HEIGHT_TOP, &det);

            // Print inference outputs
            draw_boxxes(pixels, WIDTH_TOP, HEIGHT_TOP, &objects);
            writePixelsToFrameBuffer(
                gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL), 
                pixels, 
                0, 
                0, 
                WIDTH_TOP, 
                HEIGHT_TOP);

            gfxFlushBuffers();
            gfxScreenSwapBuffers(GFX_TOP, true);
            gspWaitForVBlank();

            free(pixels);

            box_list = create_box_list();
            lv_indev_enable(indev_A, false);
            create_bottom_AB();

            break;

        default:
            break;

    }
}

lv_obj_t *create_box_list()
{

    lv_obj_t *boxxes = lv_list_create(lv_scr_act());
    lv_obj_set_size(boxxes, WIDTH_BTM, 160);
    lv_obj_align(boxxes, LV_ALIGN_TOP_MID, 0, 40);

    lv_obj_t *btn;
    lv_list_add_text(boxxes, "Press X to continue");
    char detected[40];
    sprintf(detected, "Founded %ld items", objects.num_item);
    btn = lv_list_add_btn(boxxes, LV_SYMBOL_FILE, detected);
    lv_obj_add_event_cb(btn, object_display_cb, LV_EVENT_ALL, NULL);
    lv_group_add_obj(g, btn);

    for(size_t i=0; i < objects.num_item; i++)
    {
        BoxInfo obj = objects.getItem(i, &objects);
        char list_item[40];
        int label = obj.label;
        int x1 = obj.x1;
        int x2 = obj.x2;
        int y1 = obj.y1;
        int y2 = obj.y2;     

        sprintf(list_item, "%10s [%3d,%3d,%3d,%3d]", class_names[label], x1, x2, y1, y2);
        btn = lv_list_add_btn(boxxes, LV_SYMBOL_GPS, list_item);

        lv_obj_add_event_cb(btn, object_display_cb, LV_EVENT_ALL, NULL);
        lv_group_add_obj(g, btn);
    }
    return boxxes;
}

ui_LR_t create_shoulder_button(Detector *det, void *cam_buf, bool *detecting, BoxVec *objects, lv_group_t *group, lv_obj_t *box_list)
{
    /** Create L, R button that aligned with top left and top right of screen
     * Width: 90,  Height: 30
     * */

    lv_obj_t *btn_L = lv_btn_create(lv_scr_act());    /*Add a button the current screen*/                            /*Set its position*/
    lv_obj_set_size(btn_L, 90, 30);
    lv_obj_align(btn_L, LV_ALIGN_TOP_LEFT, -10, -5);
    lv_obj_t *label_L = lv_label_create(btn_L);          /*Add a label to the button*/
    lv_label_set_text(label_L, "L  " LV_SYMBOL_VIDEO);                     /*Set the labels text*/
    lv_obj_align(label_L, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_t *btn_R = lv_btn_create(lv_scr_act());     /*Add a button the current screen*/                            /*Set its position*/
    lv_obj_set_size(btn_R, 90, 30);
    lv_obj_align(btn_R, LV_ALIGN_TOP_RIGHT, 10, -5);
    lv_obj_t *label_R = lv_label_create(btn_R);          /*Add a label to the button*/
    lv_label_set_text(label_R, LV_SYMBOL_IMAGE "  R");                     /*Set the labels text*/
    lv_obj_align(label_R, LV_ALIGN_LEFT_MID, 0, 0);
    
    lv_obj_add_event_cb(btn_L, display_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(btn_R, display_event_cb, LV_EVENT_ALL, NULL); /*Display the press stage of two button*/

    lv_obj_update_layout(btn_L);
    lv_point_t *points_array_L = (lv_point_t *) malloc(sizeof(lv_point_t) * 2);
    points_array_L[0] = (lv_point_t) {-1, -1};
    points_array_L[1] = (lv_point_t) {(btn_L->coords.x1 + btn_L->coords.x2) / 2, (btn_L->coords.y1 + btn_L->coords.y2) / 2};

    void (*functions[2])() = {virtual_L_cb, virtual_R_cb};
    static lv_indev_drv_t drv_list_LR[2];
    
    drv_list_LR[0].type = LV_INDEV_TYPE_BUTTON;
    drv_list_LR[0].read_cb = functions[0];
    lv_indev_t *l_indev = lv_indev_drv_register(&drv_list_LR[0]);
    lv_indev_set_button_points(l_indev, points_array_L);

    lv_obj_update_layout(btn_R);
    lv_point_t *points_array_R = (lv_point_t *) malloc(sizeof(lv_point_t) * 2);
    points_array_R[0] = (lv_point_t) {-1, -1};
    points_array_R[1] = (lv_point_t) {(btn_R->coords.x1 + btn_R->coords.x2) / 2, (btn_R->coords.y1 + btn_R->coords.y2) / 2};

    drv_list_LR[1].type = LV_INDEV_TYPE_BUTTON;
    drv_list_LR[1].read_cb = functions[1];
    lv_indev_t *r_indev = lv_indev_drv_register(&drv_list_LR[1]);
    lv_indev_set_button_points(r_indev, points_array_R);


    ui_LR_t output;
    output.L = btn_L;
    output.R = btn_R;
    output.point_array_L = points_array_L;
    output.point_array_R = points_array_R;

    return output;
}

lv_obj_t *put_text_example(const char *string)
{
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_radius(&style, 5);

    /*Make a gradient*/
    lv_style_set_width(&style, 150);
    lv_style_set_height(&style, LV_SIZE_CONTENT); /* This enable a flexible width and height that change 
                                                   * during inputing*/
    lv_style_set_pad_ver(&style, 20);
    lv_style_set_pad_left(&style, 5);

    /*Create an object with the new style*/
    lv_obj_t *obj = lv_obj_create(lv_scr_act());
    lv_obj_add_style(obj, &style, 0);
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 90);

    lv_obj_t *label = lv_label_create(obj);
    lv_label_set_text(label, string);

    return label;
}

void model_list_hanlder(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    Detector *det = (Detector *) lv_event_get_user_data(e);
    if(code == LV_EVENT_VALUE_CHANGED) 
    {
        destroy_detector(det);
        uint16_t model_idx = lv_dropdown_get_selected(obj);
        switch(model_idx)
        {
            case 0:
            *det = create_nanodet(320, "romfs:nanodet-plus-m_416_int8.param", "romfs:nanodet-plus-m_416_int8.bin");
            break;

            case 1:
            *det = create_fastestdet(352, "romfs:FastestDet.param", "romfs:FastestDet.bin");
            break;

            default:
            break;
        } 
        // LV_LOG_USER("Option: %s", buf);
    }
}


lv_obj_t *create_model_list(Detector *det)
{
    lv_obj_t *models = lv_dropdown_create(lv_scr_act());
    lv_dropdown_set_options(models, "NanoDet-plus\n"
                            "FastestDet");

    lv_obj_align(models, LV_ALIGN_TOP_MID, 0, -10);
    lv_obj_set_width(models, 150);
    lv_obj_add_event_cb(models, model_list_hanlder, LV_EVENT_ALL, det);
}

ui_LR_t create_bottom_btn()
{
    ui_LR_t btn_BA;

    lv_obj_t *btns[2] = {btn_BA.L, btn_BA.R};
    lv_point_t *pts_arrays[2] = {btn_BA.point_array_L, btn_BA.point_array_R};
    
    const *labels[] = {"A Detect", "B continue"};
    void (*functions[2])() = {virtual_A_cb, virtual_B_cb};
    static lv_indev_drv_t drv_list_BA[2];

    button_style_init(&btn_btm);
    lv_style_set_bg_color(&btn_btm, lv_palette_lighten(LV_PALETTE_GREY, 2));

    button_style_init(&btn_press);
    lv_style_set_bg_color(&btn_press, lv_palette_darken(LV_PALETTE_GREY, 2));

    int align[2] = {LV_ALIGN_BOTTOM_LEFT, LV_ALIGN_BOTTOM_RIGHT};
    for(int i=0; i < 2; i++)
    {
        btns[i] = lv_btn_create(lv_scr_act());
        lv_obj_remove_style_all(btns[i]);
        
        lv_obj_set_size(btns[i], lv_pct(50), 30);

        // Positions
        lv_obj_t *label = lv_label_create(btns[i]);
        lv_label_set_text(label, labels[i]);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_align(btns[i], align[i], 0, 0);

        //Styles
        lv_obj_add_style(btns[i], &btn_btm, NULL);
        lv_obj_add_style(btns[i], &btn_press, LV_STATE_PRESSED);


        // Assign click button activities
        lv_obj_update_layout(btns[i]);
        drv_list_BA[i].type = LV_INDEV_TYPE_BUTTON;
        drv_list_BA[i].read_cb = functions[i];
        lv_indev_t *BA_indev = lv_indev_drv_register(&drv_list_BA[i]);
        pts_arrays[i] = (lv_point_t *) malloc(sizeof(lv_point_t) * 2);
        pts_arrays[i][0] = (lv_point_t){-1, -1};
        pts_arrays[i][1] = (lv_point_t) {(btns[i]->coords.x1 + btns[i]->coords.x2) / 2, (btns[i]->coords.y1 + btns[i]->coords.y2) / 2};

        lv_indev_set_button_points(BA_indev, pts_arrays[i]);
    }
}

void create_bottom_A()
{
    button_style_init(&btn_btm);
    lv_style_set_bg_color(&btn_btm, lv_palette_lighten(LV_PALETTE_GREY, 2));

    button_style_init(&btn_press);
    lv_style_set_bg_color(&btn_press, lv_palette_darken(LV_PALETTE_GREY, 2));

    btn_A = lv_btn_create(lv_scr_act());
    lv_obj_remove_style_all(btn_A);

    lv_obj_set_size(btn_A, lv_pct(100), 30);

    lv_obj_t *label;
    lv_obj_t *icon_A = lv_obj_create(btn_A);
    lv_obj_set_size(icon_A, 22, 22);
    lv_obj_set_style_radius(icon_A, LV_RADIUS_CIRCLE, NULL);
    lv_obj_set_style_clip_corner(icon_A, true, NULL);
    lv_obj_set_scrollbar_mode(icon_A, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(icon_A, lv_color_hex(0xe06666), NULL);
    lv_obj_set_style_border_width(icon_A, 2, NULL);

    label = lv_label_create(icon_A);
    lv_label_set_text(label, "A");
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), NULL);
    lv_obj_center(label);

    label = lv_label_create(btn_A);
    lv_label_set_text(label, "  Detect");

    // Positions
    lv_obj_align(btn_A, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_flex_flow(btn_A, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_A,  LV_FLEX_ALIGN_CENTER,  LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    //Styles
    lv_obj_add_style(btn_A, &btn_btm, NULL);
    lv_obj_add_style(btn_A, &btn_press, LV_STATE_PRESSED);

    // Press event
    lv_obj_add_event_cb(btn_A, display_event_cb, LV_EVENT_ALL, NULL);    
    
    // Assign to encoder
    static lv_indev_drv_t drv_A;
    lv_obj_update_layout(btn_A);
    drv_A.type = LV_INDEV_TYPE_BUTTON;
    drv_A.read_cb = virtual_A_cb;
    indev_A = lv_indev_drv_register(&drv_A);
    point_array_A[0] = (lv_point_t){-1, -1};
    point_array_A[1] = (lv_point_t) {(btn_A->coords.x1 + btn_A->coords.x2) / 2, (btn_A->coords.y1 + btn_A->coords.y2) / 2};

    lv_indev_set_button_points(indev_A, point_array_A);
}

void create_bottom_AB()
{
    const *labels[] = {" continue", " Detect"};
    lv_indev_t *BA_indev[] = {&indev_B, &indev_A};
    void (*functions[2])() = { virtual_B_cb, virtual_A_cb};
    lv_point_t *pts_arrays[] = {&point_array_A, &point_array_B};
    lv_obj_t *btns[] = {&btn_B , &btn_A};
    int align[2] = {LV_ALIGN_BOTTOM_LEFT, LV_ALIGN_BOTTOM_RIGHT};
    static lv_indev_drv_t drv_B, drv_A;

    button_style_init(&btn_btm);
    lv_style_set_bg_color(&btn_btm, lv_palette_lighten(LV_PALETTE_GREY, 2));

    button_style_init(&btn_press);
    lv_style_set_bg_color(&btn_press, lv_palette_darken(LV_PALETTE_GREY, 2));


    lv_obj_t *label;

    btn_B = lv_btn_create(lv_scr_act());
    lv_obj_remove_style_all(btn_B);
    // Positions
    lv_obj_set_size(btn_B, lv_pct(50), 30);

    lv_obj_t *icon_B = lv_obj_create(btn_B);
    lv_obj_set_size(icon_B, 22, 22);
    lv_obj_set_style_radius(icon_B, LV_RADIUS_CIRCLE, NULL);
    lv_obj_set_style_clip_corner(icon_B, true, NULL);
    lv_obj_set_scrollbar_mode(icon_B, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(icon_B, lv_color_hex(0xe06666), NULL);
    lv_obj_set_style_border_width(icon_B, 2, NULL);

    label = lv_label_create(icon_B);
    lv_label_set_text(label, "B");
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), NULL);
    lv_obj_center(label);

    label = lv_label_create(btn_B);
    lv_label_set_text(label, labels[0]);
    
    // Position 
    lv_obj_align(btn_B, align[0], 0, 0);
    lv_obj_set_flex_flow(btn_B, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_B,  LV_FLEX_ALIGN_CENTER,  LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    //Styles
    lv_obj_add_style(btn_B, &btn_btm, NULL);
    lv_obj_add_style(btn_B, &btn_press, LV_STATE_PRESSED);

    
    btn_A = lv_btn_create(lv_scr_act());
    lv_obj_remove_style_all(btn_A);
    
    lv_obj_set_size(btn_A, lv_pct(50), 30);



    lv_obj_t *icon_A = lv_obj_create(btn_A);
    lv_obj_set_size(icon_A, 22, 22);
    lv_obj_set_style_radius(icon_A, LV_RADIUS_CIRCLE, NULL);
    lv_obj_set_style_clip_corner(icon_A, true, NULL);
    lv_obj_set_scrollbar_mode(icon_A, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(icon_A, lv_color_hex(0xe06666), NULL);
    lv_obj_set_style_border_width(icon_A, 2, NULL);

    label = lv_label_create(icon_A);
    lv_label_set_text(label, "A");
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), NULL);
    lv_obj_center(label);

    label = lv_label_create(btn_A);
    lv_label_set_text(label, labels[1]);

    // Positions
    lv_obj_align(btn_A, align[1], 0, 0);
    lv_obj_set_flex_flow(btn_A, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_A,  LV_FLEX_ALIGN_CENTER,  LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    //Styles
    lv_obj_add_style(btn_A, &btn_btm, NULL);
    lv_obj_add_style(btn_A, &btn_press, LV_STATE_PRESSED);


    // Assign click button activities

    lv_obj_update_layout(btn_B);
    drv_B.type = LV_INDEV_TYPE_BUTTON;
    drv_B.read_cb = virtual_B_cb;
    indev_B = lv_indev_drv_register(&drv_B);
    point_array_B[0] = (lv_point_t){-1, -1};
    point_array_B[1] = (lv_point_t) {(btn_B->coords.x1 + btn_B->coords.x2) / 2, (btn_B->coords.y1 + btn_B->coords.y2) / 2};

    lv_indev_set_button_points(indev_B, point_array_B);
;
    lv_obj_update_layout(btn_A);
    drv_A.type = LV_INDEV_TYPE_BUTTON;
    drv_A.read_cb = virtual_A_cb;
    indev_A = lv_indev_drv_register(&drv_A);
    point_array_A[0] = (lv_point_t){-1, -1};
    point_array_A[1] = (lv_point_t) {(btn_A->coords.x1 + btn_A->coords.x2) / 2, (btn_A->coords.y1 + btn_A->coords.y2) / 2};

    lv_indev_set_button_points(indev_A, point_array_A);


}
