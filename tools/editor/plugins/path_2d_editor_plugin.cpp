/*************************************************************************/
/*  path_2d_editor_plugin.cpp                                            */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                    http://www.godotengine.org                         */
/*************************************************************************/
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                 */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/
#include "path_2d_editor_plugin.h"

#include "canvas_item_editor_plugin.h"
#include "os/file_access.h"
#include "tools/editor/editor_settings.h"

void Path2DEditor::_notification(int p_what) {

	switch(p_what) {

		case NOTIFICATION_READY: {

//			button_create->set_icon( get_icon("Edit","EditorIcons"));
			//button_edit->set_icon( get_icon("MovePoint","EditorIcons"));
			//set_pressed_button(button_edit);
			//button_edit->set_pressed(true);


		} break;
		case NOTIFICATION_FIXED_PROCESS: {


		} break;
	}

}
void Path2DEditor::_node_removed(Node *p_node) {

	if(p_node==node) {
		node=NULL;
		hide();
	}

}


Vector2 Path2DEditor::snap_point(const Vector2& p_point) const {

	if (canvas_item_editor->is_snap_active()) {

		return p_point.snapped(Vector2(1,1)*canvas_item_editor->get_snap());

	} else {
		return p_point;
	}
}

bool Path2DEditor::forward_input_event(const InputEvent& p_event) {

	if (!node)
		return false;

	if (!node->get_curve().is_valid())
		return false;

	switch(p_event.type) {

		case InputEvent::MOUSE_BUTTON: {

			const InputEventMouseButton &mb=p_event.mouse_button;

			Matrix32 xform = canvas_item_editor->get_canvas_transform() * node->get_global_transform();


			Vector2 gpoint = Point2(mb.x,mb.y);
			Vector2 cpoint = xform.affine_inverse().xform(gpoint);

			//first check if a point is to be added (segment split)
			real_t grab_treshold=EDITOR_DEF("poly_editor/point_grab_radius",8);



			// Test move point!!

			if ( mb.pressed && mb.button_index==BUTTON_LEFT ) {

				Ref<Curve2D> curve = node->get_curve();

				for(int i=0;i<curve->get_point_count();i++) {

					bool pointunder=false;

					{
						Point2 p = xform.xform( curve->get_point_pos(i) );
						if (gpoint.distance_to(p) < grab_treshold ) {

							if (!mb.mod.control) {

								action=ACTION_MOVING_POINT;
								action_point=i;
								moving_from=curve->get_point_pos(i);
								moving_screen_from=gpoint;
								return true;
							} else
								pointunder=true;
						}
					}

					if (i<(curve->get_point_count()-1)) {
						Point2 p = xform.xform( curve->get_point_pos(i)+curve->get_point_out(i) );
						if (gpoint.distance_to(p) < grab_treshold ) {

							action=ACTION_MOVING_OUT;
							action_point=i;
							moving_from=curve->get_point_out(i);
							moving_screen_from=gpoint;
							return true;
						}
					}

					if (i>0) {
						Point2 p = xform.xform( curve->get_point_pos(i)+curve->get_point_in(i) );
						if (gpoint.distance_to(p) < grab_treshold ) {

							action=ACTION_MOVING_IN;
							action_point=i;
							moving_from=curve->get_point_in(i);
							moving_screen_from=gpoint;
							return true;
						}
					}

					if (pointunder)
						return true;

				}

			}

			// Test add point in empty space!

			if ( mb.pressed && mb.mod.control && mb.button_index==BUTTON_LEFT ) {

				Ref<Curve2D> curve = node->get_curve();

				undo_redo->create_action("Add Point to Curve");
				undo_redo->add_do_method(curve.ptr(),"add_point",cpoint);
				undo_redo->add_undo_method(curve.ptr(),"remove_point",curve->get_point_count());
				undo_redo->add_do_method(canvas_item_editor,"update");
				undo_redo->add_undo_method(canvas_item_editor,"update");
				undo_redo->commit_action();

				action=ACTION_MOVING_POINT;
				action_point=curve->get_point_count()-1;
				moving_from=curve->get_point_pos(action_point);
				moving_screen_from=gpoint;

				return true;
			}

			if ( !mb.pressed && mb.button_index==BUTTON_LEFT && action!=ACTION_NONE) {


				Ref<Curve2D> curve = node->get_curve();

				Vector2 new_pos = moving_from + xform.basis_xform( gpoint - moving_screen_from );
				switch(action) {

					case ACTION_MOVING_POINT: {


						undo_redo->create_action("Move Point in Curve");
						undo_redo->add_do_method(curve.ptr(),"set_point_pos",action_point,new_pos);
						undo_redo->add_undo_method(curve.ptr(),"set_point_pos",action_point,moving_from);
						undo_redo->add_do_method(canvas_item_editor,"update");
						undo_redo->add_undo_method(canvas_item_editor,"update");
						undo_redo->commit_action();

					} break;
					case ACTION_MOVING_IN: {

						undo_redo->create_action("Move In-Control in Curve");
						undo_redo->add_do_method(curve.ptr(),"set_point_in",action_point,new_pos);
						undo_redo->add_undo_method(curve.ptr(),"set_point_in",action_point,moving_from);
						undo_redo->add_do_method(canvas_item_editor,"update");
						undo_redo->add_undo_method(canvas_item_editor,"update");
						undo_redo->commit_action();

					} break;
					case ACTION_MOVING_OUT: {

						undo_redo->create_action("Move Out-Control in Curve");
						undo_redo->add_do_method(curve.ptr(),"set_point_out",action_point,new_pos);
						undo_redo->add_undo_method(curve.ptr(),"set_point_out",action_point,moving_from);
						undo_redo->add_do_method(canvas_item_editor,"update");
						undo_redo->add_undo_method(canvas_item_editor,"update");
						undo_redo->commit_action();

					} break;

				}

				action=ACTION_NONE;

				return true;
			}


#if 0
			switch(mode) {


				case MODE_CREATE: {

					if (mb.button_index==BUTTON_LEFT && mb.pressed) {


						if (!wip_active) {

							wip.clear();
							wip.push_back( snap_point(cpoint) );
							wip_active=true;
							edited_point_pos=snap_point(cpoint);
							canvas_item_editor->update();
							edited_point=1;
							return true;
						} else {

							if (wip.size()>1 && xform.xform(wip[0]).distance_to(gpoint)<grab_treshold) {
								//wip closed
								_wip_close();

								return true;
							} else {

								wip.push_back( snap_point(cpoint) );
								edited_point=wip.size();
								canvas_item_editor->update();
								return true;

								//add wip point
							}
						}
					} else if (mb.button_index==BUTTON_RIGHT && mb.pressed && wip_active) {
						_wip_close();
					}



				} break;

				case MODE_EDIT: {

					if (mb.button_index==BUTTON_LEFT) {
						if (mb.pressed) {

							if (mb.mod.control) {


								if (poly.size() < 3) {

									undo_redo->create_action("Edit Poly");
									undo_redo->add_undo_method(node,"set_polygon",poly);
									poly.push_back(cpoint);
									undo_redo->add_do_method(node,"set_polygon",poly);
									undo_redo->add_do_method(canvas_item_editor,"update");
									undo_redo->add_undo_method(canvas_item_editor,"update");
									undo_redo->commit_action();
									return true;
								}

								//search edges
								int closest_idx=-1;
								Vector2 closest_pos;
								real_t closest_dist=1e10;
								for(int i=0;i<poly.size();i++) {

									Vector2 points[2] ={ xform.xform(poly[i]),
										xform.xform(poly[(i+1)%poly.size()]) };

									Vector2 cp = Geometry::get_closest_point_to_segment_2d(gpoint,points);
									if (cp.distance_squared_to(points[0])<CMP_EPSILON2 || cp.distance_squared_to(points[1])<CMP_EPSILON2)
										continue; //not valid to reuse point

									real_t d = cp.distance_to(gpoint);
									if (d<closest_dist && d<grab_treshold) {
										closest_dist=d;
										closest_pos=cp;
										closest_idx=i;
									}


								}

								if (closest_idx>=0) {

									pre_move_edit=poly;
									poly.insert(closest_idx+1,snap_point(xform.affine_inverse().xform(closest_pos)));
									edited_point=closest_idx+1;
									edited_point_pos=snap_point(xform.affine_inverse().xform(closest_pos));
									node->set_polygon(poly);
									canvas_item_editor->update();
									return true;
								}
							} else {

								//look for points to move

								int closest_idx=-1;
								Vector2 closest_pos;
								real_t closest_dist=1e10;
								for(int i=0;i<poly.size();i++) {

									Vector2 cp =xform.xform(poly[i]);

									real_t d = cp.distance_to(gpoint);
									if (d<closest_dist && d<grab_treshold) {
										closest_dist=d;
										closest_pos=cp;
										closest_idx=i;
									}

								}

								if (closest_idx>=0) {

									pre_move_edit=poly;
									edited_point=closest_idx;
									edited_point_pos=xform.affine_inverse().xform(closest_pos);
									canvas_item_editor->update();
									return true;
								}
							}
						} else {

							if (edited_point!=-1) {

								//apply

								ERR_FAIL_INDEX_V(edited_point,poly.size(),false);
								poly[edited_point]=edited_point_pos;
								undo_redo->create_action("Edit Poly");
								undo_redo->add_do_method(node,"set_polygon",poly);
								undo_redo->add_undo_method(node,"set_polygon",pre_move_edit);
								undo_redo->add_do_method(canvas_item_editor,"update");
								undo_redo->add_undo_method(canvas_item_editor,"update");
								undo_redo->commit_action();

								edited_point=-1;
								return true;
							}
						}
					} if (mb.button_index==BUTTON_RIGHT && mb.pressed && edited_point==-1) {



						int closest_idx=-1;
						Vector2 closest_pos;
						real_t closest_dist=1e10;
						for(int i=0;i<poly.size();i++) {

							Vector2 cp =xform.xform(poly[i]);

							real_t d = cp.distance_to(gpoint);
							if (d<closest_dist && d<grab_treshold) {
								closest_dist=d;
								closest_pos=cp;
								closest_idx=i;
							}

						}

						if (closest_idx>=0) {


							undo_redo->create_action("Edit Poly (Remove Point)");
							undo_redo->add_undo_method(node,"set_polygon",poly);
							poly.remove(closest_idx);
							undo_redo->add_do_method(node,"set_polygon",poly);
							undo_redo->add_do_method(canvas_item_editor,"update");
							undo_redo->add_undo_method(canvas_item_editor,"update");
							undo_redo->commit_action();
							return true;
						}

					}



				} break;
			}


#endif
		} break;
		case InputEvent::MOUSE_MOTION: {

			const InputEventMouseMotion &mm=p_event.mouse_motion;


			if ( action!=ACTION_NONE) {

				Matrix32 xform = canvas_item_editor->get_canvas_transform() * node->get_global_transform();
				Vector2 gpoint = Point2(mm.x,mm.y);

				Ref<Curve2D> curve = node->get_curve();


				Vector2 new_pos = moving_from + xform.basis_xform( gpoint - moving_screen_from );

				switch(action) {

					case ACTION_MOVING_POINT: {

						curve->set_point_pos(action_point,new_pos);
					} break;
					case ACTION_MOVING_IN: {

						curve->set_point_in(action_point,new_pos);

					} break;
					case ACTION_MOVING_OUT: {

						curve->set_point_out(action_point,new_pos);

					} break;
				}


				canvas_item_editor->update();
				return true;
			}

#if 0
			if (edited_point!=-1 && (wip_active || mm.button_mask&BUTTON_MASK_LEFT)) {


				Matrix32 xform = canvas_item_editor->get_canvas_transform() * node->get_global_transform();

				Vector2 gpoint = Point2(mm.x,mm.y);
				edited_point_pos = snap_point(xform.affine_inverse().xform(gpoint));
				canvas_item_editor->update();

			}
#endif
		} break;
	}

	return false;
}
void Path2DEditor::_canvas_draw() {

	if (!node)
		return ;

	if (!node->get_curve().is_valid())
		return ;

	Matrix32 xform = canvas_item_editor->get_canvas_transform() * node->get_global_transform();
	Ref<Texture> handle= get_icon("EditorHandle","EditorIcons");

	Ref<Curve2D> curve = node->get_curve();

	int len = curve->get_point_count();
	RID ci = canvas_item_editor->get_canvas_item();

	for(int i=0;i<len;i++) {


		Vector2 point = xform.xform(curve->get_point_pos(i));
		handle->draw(ci,point-handle->get_size()*0.5,Color(1,1,1,0.3));

		if (i<len-1) {
			Vector2 pointout = xform.xform(curve->get_point_pos(i)+curve->get_point_out(i));
			canvas_item_editor->draw_line(point,pointout,Color(0.5,0.5,1.0,0.8),1.0);
			handle->draw(ci,pointout-handle->get_size()*0.5,Color(1,0.5,1,0.3));
		}

		if (i>0) {
			Vector2 pointin = xform.xform(curve->get_point_pos(i)+curve->get_point_in(i));
			canvas_item_editor->draw_line(point,pointin,Color(0.5,0.5,1.0,0.8),1.0);
			handle->draw(ci,pointin-handle->get_size()*0.5,Color(1,0.5,1,0.3));
		}

	}

}



void Path2DEditor::edit(Node *p_collision_polygon) {

	if (!canvas_item_editor) {
		canvas_item_editor=CanvasItemEditor::get_singleton();
	}

	if (p_collision_polygon) {

		node=p_collision_polygon->cast_to<Path2D>();
		if (!canvas_item_editor->is_connected("draw",this,"_canvas_draw"))
			canvas_item_editor->connect("draw",this,"_canvas_draw");


	} else {
		node=NULL;
		if (canvas_item_editor->is_connected("draw",this,"_canvas_draw"))
			canvas_item_editor->disconnect("draw",this,"_canvas_draw");

	}

}

void Path2DEditor::_bind_methods() {

	//ObjectTypeDB::bind_method(_MD("_menu_option"),&Path2DEditor::_menu_option);
	ObjectTypeDB::bind_method(_MD("_canvas_draw"),&Path2DEditor::_canvas_draw);

}

Path2DEditor::Path2DEditor(EditorNode *p_editor) {

	canvas_item_editor=NULL;
	editor=p_editor;
	undo_redo = editor->get_undo_redo();


	action=ACTION_NONE;
#if 0
	options = memnew( MenuButton );
	add_child(options);
	options->set_area_as_parent_rect();
	options->set_text("Polygon");
	//options->get_popup()->add_item("Parse BBCODE",PARSE_BBCODE);
	options->get_popup()->connect("item_pressed", this,"_menu_option");
#endif



}


void Path2DEditorPlugin::edit(Object *p_object) {

	collision_polygon_editor->edit(p_object->cast_to<Node>());
}

bool Path2DEditorPlugin::handles(Object *p_object) const {

	return p_object->is_type("Path2D");
}

void Path2DEditorPlugin::make_visible(bool p_visible) {

	if (p_visible) {
		collision_polygon_editor->show();
	} else {

		collision_polygon_editor->hide();
		collision_polygon_editor->edit(NULL);
	}

}

Path2DEditorPlugin::Path2DEditorPlugin(EditorNode *p_node) {

	editor=p_node;
	collision_polygon_editor = memnew( Path2DEditor(p_node) );
	editor->get_viewport()->add_child(collision_polygon_editor);

	collision_polygon_editor->set_margin(MARGIN_LEFT,200);
	collision_polygon_editor->set_margin(MARGIN_RIGHT,230);
	collision_polygon_editor->set_margin(MARGIN_TOP,0);
	collision_polygon_editor->set_margin(MARGIN_BOTTOM,10);


	collision_polygon_editor->hide();



}


Path2DEditorPlugin::~Path2DEditorPlugin()
{
}

