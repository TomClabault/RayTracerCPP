#include "mainwindow.h"
#include "timer.h"

DisplayThread::DisplayThread(MainWindow* main_window) : _main_window(main_window) {}

void DisplayThread::run()
{
    while (true)
    {
        if (_main_window->get_render_going())
        {
            while (_update_ongoing)
                std::this_thread::sleep_for(std::chrono::milliseconds(25));

            std::this_thread::sleep_for(std::chrono::milliseconds(250));

            emit update_image();
        }
    }
}

void DisplayThread::set_update_ongoing(bool ongoing)
{
    _update_ongoing = ongoing;
}

RenderThread::RenderThread(MainWindow* main_window) : _main_window(main_window) {}

//TODO mettre les DECLARATIONS des classes DisplayThread et RenderThread dans un .h plutot que dans mainwindow.h

void RenderThread::run()
{
    std::stringstream ss;
    Timer timer;

    timer.start();
    _main_window->set_render_going(true);
    if (_main_window->get_renderer().render_settings().hybrid_rasterization_tracing)
        _main_window->get_renderer().raster_trace();
    else
        _main_window->get_renderer().ray_trace();
    _main_window->set_render_going(false);
    timer.stop();
    ss << "Render time: " << timer.elapsed() << "ms" << std::endl;

    timer.start();
    _main_window->get_renderer().post_process();
    timer.stop();
    ss << "Post-processing time: " << timer.elapsed() << "ms";

    _main_window->write_to_console(ss);

    //Render finished, we're sending a signal to the main window so that it
    //updates the display
    emit update_image();
}
