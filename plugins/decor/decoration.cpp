#include <decorator.hpp>
#include <view.hpp>
#include <output.hpp>
#include <debug.hpp>
#include <plugin.hpp>
#include <core.hpp>
#include <signal-definitions.hpp>
#include "wf-decorator-protocol.h"

class gtk_frame : public wf_decorator_frame_t
{
    wf_geometry get_child_geometry(wf_geometry base)
    {
        base.x += 5;
        base.y += 40;
        base.width -= 10;
        base.height -= 45;

        return base;
    }
};

static bool begins_with(const std::string& a, const std::string& b)
{
    if (a.size() < b.size())
        return false;

    size_t i = 0;
    while(i < b.size() && a[i] == b[i]) ++i;
    return i == b.size();
}

class gtk_decorator : public decorator_base_t
{
    const std::string gtk_decorator_prefix = "__wf_decorator:";

    public:

    virtual bool is_decoration_window(std::string title)
    {
        log_info("got with title %s %d", title.c_str(), begins_with(title, gtk_decorator_prefix));
        return begins_with(title, gtk_decorator_prefix);
    }

    virtual void decoration_ready(std::unique_ptr<wayfire_view_t> decor_window)
    {
        auto title = decor_window->get_title();
        uint32_t id = std::stoul(title.substr(gtk_decorator_prefix.size()));

        log_info("decor ready for %s %d", title.c_str(), id);
        auto view = core->find_view(id);
        assert(view);

        auto frame = new gtk_frame();
        view->set_decoration(std::move(decor_window), std::unique_ptr<wf_decorator_frame_t>(frame));
    }
} decorator;

/* TODO: implement update_borders */
void update_borders(struct wl_client *client,
                    struct wl_resource *resource,
                    struct wl_resource *decoration,
                    uint32_t top,
                    uint32_t bottom,
                    uint32_t left,
                    uint32_t right)
{
    /* TODO: update borders */
}

const struct wf_decorator_manager_interface decorator_implementation =
{
    update_borders
};

wl_resource *decorator_resource = NULL;
void unbind_decorator(wl_resource *resource)
{
    decorator_resource = NULL;
}

void bind_decorator(wl_client *client, void *data,
                    uint32_t version, uint32_t id)
{
    log_info("bind");
    auto resource = wl_resource_create(client,
                                       &wf_decorator_manager_interface,
                                       1, id);
    /* TODO: track active clients */
    wl_resource_set_implementation(resource, &decorator_implementation,
                                   NULL, NULL);
    decorator_resource = resource;
}

class wayfire_decoration : public wayfire_plugin_t
{
    signal_callback_t view_created;

    public:
    void init(wayfire_config *config)
    {
        if (!core->set_decorator(&decorator))
            return;

        wl_global_create(core->display,
                         &wf_decorator_manager_interface,
                         1, NULL, bind_decorator);

        view_created = [=] (signal_data *data)
        {
            new_view(get_signaled_view(data));
        };

        output->connect_signal("create-view", &view_created);
    }

    void new_view(wayfire_view view)
    {
        log_info("new view %p", decorator_resource);
        if (decorator_resource)
        {
            wf_decorator_manager_send_create_new_decoration(decorator_resource,
                                                            view->get_id());
        }
    }
};

extern "C"
{
    wayfire_plugin_t *newInstance()
    {
        return new wayfire_decoration;
    }
}

