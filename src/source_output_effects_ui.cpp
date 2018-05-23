#include "source_output_effects_ui.hpp"

SourceOutputEffectsUi::SourceOutputEffectsUi(
    BaseObjectType* cobject,
    const Glib::RefPtr<Gtk::Builder>& refBuilder,
    std::shared_ptr<SourceOutputEffects> soe_ptr)
    : EffectsBaseUi(cobject, refBuilder, soe_ptr->pm),
      soe(soe_ptr),
      settings(
          Gio::Settings::create("com.github.wwmm.pulseeffects.sourceoutputs")),
      limiter_ui(LimiterUi::create(
          "com.github.wwmm.pulseeffects.sourceoutputs.limiter")),
      compressor_ui(CompressorUi::create(
          "com.github.wwmm.pulseeffects.sourceoutputs.compressor")),
      filter_ui(FilterUi::create(
          "com.github.wwmm.pulseeffects.sourceoutputs.filter")),
      equalizer_ui(EqualizerUi::create(
          "com.github.wwmm.pulseeffects.sourceoutputs.equalizer")),
      reverb_ui(ReverbUi::create(
          "com.github.wwmm.pulseeffects.sourceoutputs.reverb")) {
    level_meters_connections();
    populate_listbox();
    populate_stack();
    up_down_connections();

    listbox->set_sort_func([=](Gtk::ListBoxRow* row1, Gtk::ListBoxRow* row2) {
        auto name1 = row1->get_name();
        auto name2 = row2->get_name();

        auto order = Glib::Variant<std::vector<std::string>>();

        settings->get_value("plugins", order);

        auto vorder = order.get();

        auto r1 = std::find(std::begin(vorder), std::end(vorder), name1);
        auto r2 = std::find(std::begin(vorder), std::end(vorder), name2);

        auto idx1 = r1 - vorder.begin();
        auto idx2 = r2 - vorder.begin();

        // we do not want the applications row to be moved

        if (name1 == std::string("applications")) {
            return -1;
        }

        if (name2 == std::string("applications")) {
            return 1;
        }

        if (idx1 < idx2) {
            return -1;
        } else if (idx1 > idx2) {
            return 1;
        } else {
            return 0;
        }
    });

    connections.push_back(settings->signal_changed("plugins").connect(
        [=](auto key) { listbox->invalidate_sort(); }));
}

SourceOutputEffectsUi::~SourceOutputEffectsUi() {
    for (auto c : connections) {
        c.disconnect();
    }
}

std::unique_ptr<SourceOutputEffectsUi> SourceOutputEffectsUi::create(
    std::shared_ptr<SourceOutputEffects> soe) {
    auto builder = Gtk::Builder::create_from_resource(
        "/com/github/wwmm/pulseeffects/effects_base.glade");

    SourceOutputEffectsUi* soe_ui = nullptr;

    builder->get_widget_derived("widgets_box", soe_ui, soe);

    return std::unique_ptr<SourceOutputEffectsUi>(soe_ui);
}

void SourceOutputEffectsUi::level_meters_connections() {
    // limiter level meters connections

    connections.push_back(soe->limiter->input_level.connect(
        sigc::mem_fun(*limiter_ui, &LimiterUi::on_new_input_level)));
    connections.push_back(soe->limiter->output_level.connect(
        sigc::mem_fun(*limiter_ui, &LimiterUi::on_new_output_level)));
    connections.push_back(soe->limiter->attenuation.connect(
        sigc::mem_fun(*limiter_ui, &LimiterUi::on_new_attenuation)));

    // compressor level meters connections

    connections.push_back(soe->compressor_input_level.connect(
        sigc::mem_fun(*compressor_ui, &CompressorUi::on_new_input_level_db)));
    connections.push_back(soe->compressor_output_level.connect(
        sigc::mem_fun(*compressor_ui, &CompressorUi::on_new_output_level_db)));
    connections.push_back(soe->compressor->compression.connect(
        sigc::mem_fun(*compressor_ui, &CompressorUi::on_new_compression)));

    // filter level meters connections

    connections.push_back(soe->filter->input_level.connect(
        sigc::mem_fun(*filter_ui, &FilterUi::on_new_input_level)));
    connections.push_back(soe->filter->output_level.connect(
        sigc::mem_fun(*filter_ui, &FilterUi::on_new_output_level)));

    // equalizer level meters connections

    connections.push_back(soe->equalizer_input_level.connect(
        sigc::mem_fun(*equalizer_ui, &EqualizerUi::on_new_input_level_db)));
    connections.push_back(soe->equalizer_output_level.connect(
        sigc::mem_fun(*equalizer_ui, &EqualizerUi::on_new_output_level_db)));

    // reverb level meters connections

    connections.push_back(soe->reverb->input_level.connect(
        sigc::mem_fun(*reverb_ui, &ReverbUi::on_new_input_level)));
    connections.push_back(soe->reverb->output_level.connect(
        sigc::mem_fun(*reverb_ui, &ReverbUi::on_new_output_level)));
}

void SourceOutputEffectsUi::populate_listbox() {
    auto plugins_order = Glib::Variant<std::vector<std::string>>();

    settings->get_value("plugins", plugins_order);

    for (auto name : plugins_order.get()) {
        if (name == std::string("limiter")) {
            add_to_listbox(limiter_ui);
        } else if (name == std::string("compressor")) {
            add_to_listbox(compressor_ui);
        } else if (name == std::string("filter")) {
            add_to_listbox(filter_ui);
        } else if (name == std::string("equalizer")) {
            add_to_listbox(equalizer_ui);
        } else if (name == std::string("reverb")) {
            add_to_listbox(reverb_ui);
        }
    }
}
void SourceOutputEffectsUi::populate_stack() {
    stack->add(*limiter_ui, std::string("limiter"));
    stack->add(*compressor_ui, std::string("compressor"));
    stack->add(*filter_ui, std::string("filter"));
    stack->add(*equalizer_ui, std::string("equalizer"));
    stack->add(*reverb_ui, std::string("reverb"));
}

void SourceOutputEffectsUi::up_down_connections() {
    auto on_up = [=](auto p) {
        auto order = Glib::Variant<std::vector<std::string>>();

        settings->get_value("plugins", order);

        auto vorder = order.get();

        auto r = std::find(std::begin(vorder), std::end(vorder), p->name);

        if (r != std::begin(vorder)) {
            std::iter_swap(r, r - 1);

            settings->set_string_array("plugins", vorder);
        }
    };

    auto on_down = [=](auto p) {
        auto order = Glib::Variant<std::vector<std::string>>();

        settings->get_value("plugins", order);

        auto vorder = order.get();

        auto r = std::find(std::begin(vorder), std::end(vorder), p->name);

        if (r != std::end(vorder) - 1) {
            std::iter_swap(r, r + 1);

            settings->set_string_array("plugins", vorder);
        }
    };

    connections.push_back(limiter_ui->plugin_up->signal_clicked().connect(
        [=]() { on_up(limiter_ui); }));
    connections.push_back(limiter_ui->plugin_down->signal_clicked().connect(
        [=]() { on_down(limiter_ui); }));

    connections.push_back(compressor_ui->plugin_up->signal_clicked().connect(
        [=]() { on_up(compressor_ui); }));
    connections.push_back(compressor_ui->plugin_down->signal_clicked().connect(
        [=]() { on_down(compressor_ui); }));

    connections.push_back(filter_ui->plugin_up->signal_clicked().connect(
        [=]() { on_up(filter_ui); }));
    connections.push_back(filter_ui->plugin_down->signal_clicked().connect(
        [=]() { on_down(filter_ui); }));

    connections.push_back(equalizer_ui->plugin_up->signal_clicked().connect(
        [=]() { on_up(equalizer_ui); }));
    connections.push_back(equalizer_ui->plugin_down->signal_clicked().connect(
        [=]() { on_down(equalizer_ui); }));

    connections.push_back(reverb_ui->plugin_up->signal_clicked().connect(
        [=]() { on_up(reverb_ui); }));
    connections.push_back(reverb_ui->plugin_down->signal_clicked().connect(
        [=]() { on_down(reverb_ui); }));
}

void SourceOutputEffectsUi::reset() {
    limiter_ui->reset();
    compressor_ui->reset();
    filter_ui->reset();
    equalizer_ui->reset();
    reverb_ui->reset();
}
