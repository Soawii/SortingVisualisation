#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <ctime>
#include <string>
#include <climits>
#include <deque>

int WIDTH = 1200, HEIGHT = 800;
int SORT_WIDTH = WIDTH * 0.85;
int IMAGE_HEIGHT = 700;

sf::Color BACKGROUND_COLOR(230, 230, 230);
sf::Color ACTION_COLOR(255, 0, 0);
sf::Color NORMAL_COLOR(200, 200, 255);
sf::Color LIGHT_BLUE(200, 200, 255);
sf::Color WHITE(255, 255, 255);
sf::Color DONE_COLOR(0, 255, 0);
int ACTIONS_PER_FRAME = 20;
int SOUND_COEFF = 20;
int POPULATION = 100;
bool IS_ANIMATION = false;
enum class Sort { bubble, coctail_shaker, insertion, shell, quick, merge, heap, counting, radix };
enum class Pressable { button, slider, textbox, dropdown, checkbox };

// Class used for handling button priority so 2 buttons don't get pressed at once
class ButtonPriority
{
public:
    ButtonPriority(Pressable pressable_, void* button_ptr_, int priority_)
    {
        pressable = pressable_;
        button_ptr = button_ptr_;
        priority = priority_;
    }
    void* button_ptr;
    Pressable pressable;
    int priority = 0;
};

std::vector<ButtonPriority> all_buttons;

// Class handling the main array, its sort functions and everything else
class MainVector
{
public:
    MainVector(size_t population, sf::RenderWindow* new_window)
    {
        font.loadFromFile("regular.ttf");
        window = new_window;
        populate_vector(population);

        info_text.resize(3);
        stat_text.resize(3);
        time_text.resize(3);

        // Set positions and sizes of info texts
        for (int i = 0; i < 3; i++)
        {
            info_text[i].setFont(font);
            info_text[i].setCharacterSize(16);
            info_text[i].setFillColor(sf::Color(30, 30, 30));
            info_text[i].setPosition(10, 10 + i * 50);
            info_text[i].setString(sort_info[int(sort_chosen[i])]);

            stat_text[i].setFont(font);
            stat_text[i].setCharacterSize(16);
            stat_text[i].setFillColor(sf::Color(30, 30, 30));
            stat_text[i].setPosition(10, 10 + 18 + i * 50);

            time_text[i].setFont(font);
            time_text[i].setCharacterSize(16);
            time_text[i].setFillColor(sf::Color(30, 30, 30));
            time_text[i].setPosition(20 + info_text[i].getLocalBounds().left + info_text[i].getLocalBounds().width, 10 + i * 50);
            time_text[i].setString("Time: ");
        }

        sound_buffer.loadFromFile("laser.wav");

        // Cuts the sound to 4% of its length, easiest way i found to do it :(
        std::size_t start_cut = sound_buffer.getSampleCount() * 0.46, end_cut = sound_buffer.getSampleCount() * 0.5;
        trimmed_buffer.loadFromSamples(sound_buffer.getSamples() + start_cut, sound_buffer.getSampleCount() - start_cut - end_cut, sound_buffer.getChannelCount(), sound_buffer.getSampleRate());

        // Pitches of the sound, bigger element in the array = higher pitch used for sound
        beep_pitches.resize(1000, 0);
        for (int i = 0; i < 1000; i++)
        {
            beep_pitches[i] = 0.2 + i * 0.0012;
            beep_pitches[i] = 0.2 + i * 0.0012;
        }
    }

    // Populates the array with (population) elements, fills it with random elements depending on MAX_ELEMENT value.
    // Additionally sets RectangleShapes or Vertexes (based on population size) to their corresponding array elements
    void populate_vector(size_t population)
    {
        NORMAL_COLOR = image_sort ? WHITE : LIGHT_BLUE;
        if (!(can_populate || stopped)) return;
        element_width = SORT_WIDTH / (int(population) * 1.0);
        if (element_width > 3) vertices_used = false;
        else vertices_used = true;

        new_idxs.resize(population);
        for (int i = 0; i < population; i++) new_idxs[i] = i;

        orig_main_vector.resize(population);
        for (int i = 0; i < orig_rectangle_vector.size(); i++)
        {
            if (orig_rectangle_vector[i] != nullptr)  delete orig_rectangle_vector[i];
            if (rectangle_vector[i] != nullptr) delete rectangle_vector[i];
        }

        if (vertices_used)
        {
            orig_rectangle_vector.clear();
            rectangle_vector.clear();
        }
        else
        {
            orig_rectangle_vector.resize(population, nullptr);
            rectangle_vector.resize(population, nullptr);
        }

        double ratio = double(population) / SORT_WIDTH;
        vertex_vector.setPrimitiveType(ratio >= 1 ? sf::Lines : sf::Quads);
        orig_vertex_vector.setPrimitiveType(ratio >= 1 ? sf::Lines : sf::Quads);
        if (vertices_used)
        {
            vertex_vector.resize(population * 2 * (ratio < 1 ? 2 : 1));
            orig_vertex_vector.resize(population * 2 * (ratio < 1 ? 2 : 1));
        }

        for (int i = 0; i < population; i++)
        {
            if (MAX_ELEMENT >= RAND_MAX)
            {
                orig_main_vector[i] = int(((double)std::rand() / RAND_MAX) * (MAX_ELEMENT - 1)) + 1;
            }
            else
            {
                orig_main_vector[i] = (std::rand() % MAX_ELEMENT + 1);
            }

            if (!vertices_used)
            {
                orig_rectangle_vector[i] = new sf::RectangleShape();
                orig_rectangle_vector[i]->setSize(sf::Vector2f(element_width - 1, image_sort ? IMAGE_HEIGHT : (double(orig_main_vector[i]) / (MAX_ELEMENT * 1.1) * HEIGHT)));
                orig_rectangle_vector[i]->setPosition(sf::Vector2f(i * element_width, HEIGHT - orig_rectangle_vector[i]->getSize().y));
                orig_rectangle_vector[i]->setFillColor(NORMAL_COLOR);
                orig_rectangle_vector[i]->setOutlineThickness(int(1));
                orig_rectangle_vector[i]->setOutlineColor(sf::Color(30, 30, 30));

                rectangle_vector[i] = new sf::RectangleShape();
                *rectangle_vector[i] = *orig_rectangle_vector[i];
            }
            else
            {
                int add = vertex_vector.getPrimitiveType() == sf::Lines ? 2 : 4;

                orig_vertex_vector[i * add] = sf::Vertex(sf::Vector2f(i * element_width, image_sort ? HEIGHT - IMAGE_HEIGHT : HEIGHT - (double(orig_main_vector[i]) / (MAX_ELEMENT * 1.1) * HEIGHT)), NORMAL_COLOR, sf::Vector2f(10, 10));
                orig_vertex_vector[i * add + 1] = sf::Vertex(sf::Vector2f(i * element_width, HEIGHT), NORMAL_COLOR, sf::Vector2f(20, 20));
                vertex_vector[i * add] = orig_vertex_vector[i * add];
                vertex_vector[i * add + 1] = orig_vertex_vector[i * add + 1];
                if (orig_vertex_vector.getPrimitiveType() == sf::Quads)
                {
                    orig_vertex_vector[i * add + 2] = sf::Vertex(sf::Vector2f((i + 1) * element_width, HEIGHT), NORMAL_COLOR, sf::Vector2f(10, 10));
                    orig_vertex_vector[i * add + 3] = sf::Vertex(sf::Vector2f((i + 1) * element_width, image_sort ? HEIGHT - IMAGE_HEIGHT : HEIGHT - (double(orig_main_vector[i]) / (MAX_ELEMENT * 1.1) * HEIGHT)), NORMAL_COLOR, sf::Vector2f(10, 0));
                    vertex_vector[i * add + 2] = orig_vertex_vector[i * add + 2];
                    vertex_vector[i * add + 3] = orig_vertex_vector[i * add + 3];
                }
            }
        }
        main_vector.resize(orig_main_vector.size());
        for (int i = 0; i < main_vector.size(); i++)
        {
            main_vector[i] = std::make_pair(orig_main_vector[i], i);
        }

        can_populate = true;
        can_sort = true;
        sorted = false;
        done_phase = false;
        changes.clear();
        ACTIONS_DONE = 0;
        unstop();
    }

    // Does the same thing as populate_vector, except array values stay the same
    void repopulate_vector()
    {
        int add = vertex_vector.getPrimitiveType() == sf::Lines ? 2 : 4;
        for (int i = 0; i < main_vector.size(); i++)
        {
            main_vector[i].first = orig_main_vector[i];
            main_vector[i].second = i;
            if (vertices_used)
            {
                for (int k = 0; k < add; k++)
                {
                    vertex_vector[i * add + k] = orig_vertex_vector[i * add + k];
                }
            }
            else
            {
                *(rectangle_vector[i]) = *(orig_rectangle_vector[i]);
            }
        }
        new_idxs.resize(main_vector.size());
        for (int i = 0; i < main_vector.size(); i++) new_idxs[i] = i;

        can_populate = true;
        can_sort = true;
        sorted = false;
        done_phase = false;

        changes.clear();
        ACTIONS_DONE = 0;
        unstop();
    }

    // Sets the sort algorithm for #(sort_number) sort, changes info and time texts according to the algorithm set.
    void set_sort(Sort new_sort, int sort_number)
    {
        if (!((can_populate && can_sort) || stopped)) return;
        sort_chosen[sort_number] = new_sort;
        info_text[sort_number].setString(sort_info[int(sort_chosen[sort_number])]);
        time_text[sort_number].setPosition(20 + info_text[sort_number].getLocalBounds().left + info_text[sort_number].getLocalBounds().width, 10 + sort_number * 50);
    }

    Sort get_sort(int sort_number) const
    {
        return sort_chosen[sort_number];
    }

    int get_size() const
    {
        return main_vector.size();
    }

    // Swaps elements in the main array, pushes info into changes array
    void swap(int i, int j)
    {
        if (IS_ANIMATION) changes.push_back({ i, j, (int)ActionType::swap });
        else writes[current_sort] += 2;
        std::swap(main_vector[i], main_vector[j]);
    }

    // Sets value in main_vector[dest] equal to value in orig_main_vector[source], pushes info into changes array
    void set(int dest, int source)
    {
        main_vector[dest].first = orig_main_vector[source];
        main_vector[dest].second = source;
        if (IS_ANIMATION) changes.push_back({ dest, source, (int)ActionType::set });
        else writes[current_sort]++;
    }

    // Swaps rectangles or vertices with indexes i and j
    void swap_rectangles(int i, int j)
    {
        if (!vertices_used)
        {
            sf::RectangleShape* temp2 = rectangle_vector[i];
            rectangle_vector[i] = rectangle_vector[j];
            rectangle_vector[j] = temp2;
            rectangle_vector[i]->setPosition(sf::Vector2f(i * element_width, HEIGHT - rectangle_vector[i]->getSize().y));
            rectangle_vector[j]->setPosition(sf::Vector2f(j * element_width, HEIGHT - rectangle_vector[j]->getSize().y));
        }
        else
        {

            int add = vertex_vector.getPrimitiveType() == sf::Lines ? 2 : 4;
            int idx_i = i * add, idx_j = j * add;

            sf::Vector2f temp = vertex_vector[idx_i].position;

            vertex_vector[idx_i].position.y = vertex_vector[idx_j].position.y;
            vertex_vector[idx_j].position.y = temp.y;

            sf::Vector2f temp_text = vertex_vector[idx_i].texCoords;
            vertex_vector[idx_i].texCoords = vertex_vector[idx_j].texCoords;
            vertex_vector[idx_j].texCoords = temp_text;
            temp_text = vertex_vector[idx_i + 1].texCoords;
            vertex_vector[idx_i + 1].texCoords = vertex_vector[idx_j + 1].texCoords;
            vertex_vector[idx_j + 1].texCoords = temp_text;

            if (add == 4)
            {
                temp = vertex_vector[idx_i + 3].position;
                vertex_vector[idx_i + 3].position.y = vertex_vector[idx_j + 3].position.y;
                vertex_vector[idx_j + 3].position.y = temp.y;

                temp_text = vertex_vector[idx_i + 2].texCoords;
                vertex_vector[idx_i + 2].texCoords = vertex_vector[idx_j + 2].texCoords;
                vertex_vector[idx_j + 2].texCoords = temp_text;
                temp_text = vertex_vector[idx_i + 3].texCoords;
                vertex_vector[idx_i + 3].texCoords = vertex_vector[idx_j + 3].texCoords;
                vertex_vector[idx_j + 3].texCoords = temp_text;
            }
        }
    }

    bool (*less_than)(int, int) = [](int a, int b) {return a < b; };
    bool (*more_than)(int, int) = [](int a, int b) {return a > b; };

    // Compares elements in the main array with indexes i and j based on the cmp function, pushes info into changes array
    bool compare(int i, int j, bool (*cmp)(int, int))
    {
        if (IS_ANIMATION) changes.push_back({ i, j, (int)ActionType::comparison });
        else comparisons[current_sort]++;
        return cmp(main_vector[i].first, main_vector[j].first);
    }

    void stop()
    {
        if (sorted || can_populate || can_sort) return;
        stopped = true;
    }

    void unstop()
    {
        if (!stopped) return;
        stopped = false;
    }

    // Main draw function
    void draw()
    {
        int min_sound = 1000000000, max_sound = -1;
        double avg = 0;
        int cnt = 0;

        // If we've done more actions than there are elements in the changes array, we are done with the main part of the animation.
        // Now all there is left to do is the "Done Animation" or turn all rectangles or vertices green.
        if (ACTIONS_DONE >= changes.size())
        {
            if (changes.size() > 0)
            {
                changes.clear();
                if (!done_phase)
                {
                    for (int i = 0; i < main_vector.size(); i++)
                    {
                        changes.push_back({ i, i, (int)ActionType::green_set });
                    }
                    done_phase = true;
                }
                else
                {
                    if (image_sort)
                    {
                        for (int i = 0; i < main_vector.size(); i++)
                        {
                            if (!vertices_used) rectangle_vector[i]->setFillColor(NORMAL_COLOR);
                            else
                            {
                                int add = vertex_vector.getPrimitiveType() == sf::Lines ? 2 : 4;
                                for (int k = 0; k < add; k++) vertex_vector[i * add + k].color = NORMAL_COLOR;
                            }
                        }
                    }
                    sorted = true;
                    can_sort = true;
                    can_populate = true;
                }
            }
            ACTIONS_DONE = 0;
        }

        // Animate the amount of actions set by the value of ACTIONS_PER_FRAME
        int ACTIONS_TO_DO = -1;
        if (ACTIONS_DONE < changes.size())
        {
            ACTIONS_TO_DO = done_phase ? std::max(1, int(main_vector.size() / 50)) : (stopped ? 0 : ACTIONS_PER_FRAME);
            for (int idx = ACTIONS_DONE; idx < std::min(int(changes.size()), ACTIONS_DONE + ACTIONS_TO_DO); idx++)
            {
                int i = changes[idx][0], j = changes[idx][1];
                int add = vertex_vector.getPrimitiveType() == sf::Lines ? 2 : 4;
                int i_idx = (i * add), j_idx = (j * add);

                // Perform different operations based on the type of the current action
                if (changes[idx][2] == (int)ActionType::comparison) comparisons[current_sort]++;
                else if (changes[idx][2] == (int)ActionType::swap)
                {
                    writes[current_sort] += 2;
                    swap_rectangles(i, j);
                }
                else if (changes[idx][2] == (int)ActionType::set)
                {
                    writes[current_sort]++;
                    if (vertices_used)
                    {
                        vertex_vector[i_idx].position.y = orig_vertex_vector[j_idx].position.y;

                        vertex_vector[i_idx].texCoords = orig_vertex_vector[j_idx].texCoords;
                        vertex_vector[i_idx + 1].texCoords = orig_vertex_vector[j_idx + 1].texCoords;
                        if (vertex_vector.getPrimitiveType() == sf::Quads)
                        {
                            vertex_vector[i_idx + 3].position.y = orig_vertex_vector[j_idx + 3].position.y;
                            vertex_vector[i_idx + 2].texCoords = orig_vertex_vector[j_idx + 2].texCoords;
                            vertex_vector[i_idx + 3].texCoords = orig_vertex_vector[j_idx + 3].texCoords;
                        }
                    }
                    else
                    {
                        *(rectangle_vector[i]) = *(orig_rectangle_vector[j]);
                        rectangle_vector[i]->setPosition(sf::Vector2f(i * element_width, HEIGHT - rectangle_vector[i]->getSize().y));
                    }
                }
                else if (changes[idx][2] == (int)ActionType::green_set)
                {
                    if (!vertices_used) rectangle_vector[i]->setFillColor(DONE_COLOR);
                    else
                    {
                        for (int k = 0; k < add; k++) vertex_vector[i_idx + k].color = DONE_COLOR;
                    }
                }
                
                // If action is not of type green_set, set the colors of the current rectangles or vertices to the ACTION_COLOR
                if (changes[idx][2] != (int)ActionType::green_set)
                {
                    if (!vertices_used) rectangle_vector[i]->setFillColor(ACTION_COLOR);
                    else
                    {
                        for (int k = 0; k < add; k++) vertex_vector[i_idx + k].color = ACTION_COLOR;
                    }
                    if (changes[idx][2] != 2)
                    {
                        if (!vertices_used) rectangle_vector[j]->setFillColor(ACTION_COLOR);
                        else
                        {
                            for (int k = 0; k < add; k++) vertex_vector[j_idx + k].color = ACTION_COLOR;
                        }
                    }
                }

                // Add height of the current rectangle of vertex to the avg value, to determine the pitch of the sound played on this frame
                avg += vertices_used ? HEIGHT - vertex_vector[i_idx].position.y : rectangle_vector[i]->getSize().y;
                cnt++;
                if (i != j)
                {
                    avg += vertices_used ? HEIGHT - vertex_vector[j_idx].position.y : rectangle_vector[j]->getSize().y;
                    cnt++;
                }
            }
        }
        if (!vertices_used)
        {
            // If rectangles are used, draw all of them on the window
            for (int i = 0; i < rectangle_vector.size(); i++) window->draw(*rectangle_vector[i]);
        }
        else
        {
            // If vertices are used, we only want to draw the ones that are the first to lay on each pixel
            // This is done so we don't draw up to a million vertices but only the amount needed when array size gets too big
            int add = vertex_vector.getPrimitiveType() == sf::Lines ? 2 : 4;
            if (add == 2)
            {
                for (int i = 0; i < vertex_vector.getVertexCount(); i += 2)
                {
                    if (i < 2 || (int(vertex_vector[i].position.x) != int(vertex_vector[i - 2].position.x)))
                    {
                        float prev_pos1 = vertex_vector[i].position.x;
                        vertex_vector[i].position.x = int(vertex_vector[i].position.x);
                        vertex_vector[i + 1].position.x = int(vertex_vector[i].position.x);
                        if (image_sort && (sorted || !can_populate)) window->draw(&vertex_vector[i], add, sf::Lines, &sort_image_text);
                        else window->draw(&vertex_vector[i], add, sf::Lines);
                        vertex_vector[i].position.x = prev_pos1;
                        vertex_vector[i + 1].position.x = prev_pos1;
                    }
                }
            }
            else
            {
                if (image_sort && (sorted || !can_populate)) window->draw(&vertex_vector[0], vertex_vector.getVertexCount(), sf::Quads, &sort_image_text);
                else window->draw(&vertex_vector[0], vertex_vector.getVertexCount(), sf::Quads);
            }
        }

        // After we finished drawing, change the colors of changed rectangles or vertices back to normal
        if (ACTIONS_DONE < changes.size())
        {
            for (int idx = ACTIONS_DONE; idx < std::min(int(changes.size()), ACTIONS_DONE + ACTIONS_TO_DO); idx++)
            {
                int i = changes[idx][0], j = changes[idx][1];
                int add = vertex_vector.getPrimitiveType() == sf::Lines ? 2 : 4;
                int i_idx = (i * add), j_idx = (j * add);
                if (changes[idx][2] != (int)ActionType::green_set)
                {
                    if (!vertices_used) rectangle_vector[i]->setFillColor(NORMAL_COLOR);
                    else
                    {
                        for (int k = 0; k < add; k++) vertex_vector[i_idx + k].color = NORMAL_COLOR;
                    }
                    if (changes[idx][2] != (int)ActionType::set)
                    {
                        if (!vertices_used) rectangle_vector[j]->setFillColor(NORMAL_COLOR);
                        else
                        {
                            for (int k = 0; k < add; k++) vertex_vector[j_idx + k].color = NORMAL_COLOR;
                        }
                    }
                }
            }
        }

        // Update text info of all sorts
        for (int i = 0; i < (IS_ANIMATION ? 1 : 3); i++)
        {
            stat_text[i].setString("Comparisons: " + std::to_string(comparisons[i]) + "   Writes: " + std::to_string(writes[i]));
            window->draw(stat_text[i]);
            window->draw(info_text[i]);
            window->draw(time_text[i]);
        }

        // Delete all sounds that stopped playing from the deque
        while (!sound_deque.empty() && sound_deque.front()->getStatus() == sf::Sound::Stopped)
        {
            delete sound_deque.front();
            sound_deque.pop_front();
        }

        // Add a sound with a pitch based on the value of elements accessed on this frame
        if (ACTIONS_TO_DO > 0)
        {
            int ratio = int((avg / cnt / HEIGHT) * 999);
            if (image_sort) ratio = rand() % 400 + 300;
            sf::Sound* sound = new sf::Sound(trimmed_buffer);
            sound->setVolume(SOUND_COEFF / 7.0);
            sound->setPitch(beep_pitches[ratio]);
            sound->play();
            sound_deque.push_back(sound);
            ACTIONS_DONE += ACTIONS_TO_DO;
        }
    }

    // Main sort method
    void sort()
    {
        // If image mode is selected, reload the image
        if (image_sort)
        {
            sort_image.loadFromFile("images\\image" + std::to_string(rand() % 3) + ".jpeg");
            sort_image_text.loadFromImage(sort_image);
            if (image_sort) populate_vector(main_vector.size());
        }

        // Populate main array with appropriate size if the size set is too big for one of the chosen sort algorithms
        if (IS_ANIMATION)
        {
            if (main_vector.size() > sort_to_max_limited[int(sort_chosen[0])]) populate_vector(sort_to_max_limited[int(sort_chosen[0])]);
            else repopulate_vector();
        }
        else
        {
            int min_max_element = std::min({ sort_to_max_unlimited[int(sort_chosen[0])], sort_to_max_unlimited[int(sort_chosen[1])] , sort_to_max_unlimited[int(sort_chosen[2])] });
            if (main_vector.size() > min_max_element) populate_vector(min_max_element);
            else repopulate_vector();
        }

        int sorts = (IS_ANIMATION ? 1 : 3);
        IS_ANIMATION = false;
        PURE_SORTING = true;

        // First sort done with PURE_SORTING set to true, to measure accurate time of sorts performed.
        // PURE_SORTING set to true makes comparisons and swaps not call additional functions causing significant speed loss.
        for (int i = 0; i < sorts; i++)
        {
            current_sort = i;
            repopulate_vector();
            sf::Clock clock;
            comparisons[i] = 0;
            writes[i] = 0;

            int start = clock.getElapsedTime().asMilliseconds();

            if (sort_chosen[i] == Sort::bubble) bubble_sort();
            else if (sort_chosen[i] == Sort::coctail_shaker) coctail_shaker_sort();
            else if (sort_chosen[i] == Sort::insertion) insertion_sort();
            else if (sort_chosen[i] == Sort::shell) shell_sort();
            else if (sort_chosen[i] == Sort::quick) quick_sort();
            else if (sort_chosen[i] == Sort::merge) merge_sort();
            else if (sort_chosen[i] == Sort::heap) heap_sort();
            else if (sort_chosen[i] == Sort::counting) counting_sort();
            else if (sort_chosen[i] == Sort::radix) radix_sort();

            int end = clock.getElapsedTime().asMilliseconds();
            time_text[i].setString("Time: " + std::to_string(end - start) + " ms.");
        }

        IS_ANIMATION = (sorts == 1 ? true : false);
        PURE_SORTING = false;

        if (image_sort)
        {
            float pixels_per_rect = (float)sort_image.getSize().x / (float)main_vector.size();
            if (IS_ANIMATION)
            {
                while (texts.size() < main_vector.size()) texts.push_back(new sf::Texture);
                if (!vertices_used)
                {
                    for (int i = 0; i < main_vector.size(); i++)
                    {
                        texts[i]->loadFromImage(sort_image, sf::IntRect(sf::Vector2i(pixels_per_rect * i, 0), sf::Vector2i(pixels_per_rect, sort_image.getSize().y)));
                        orig_rectangle_vector[main_vector[i].second]->setTexture(texts[i]);
                    }
                }
                else
                {
                    int temp = orig_vertex_vector.getPrimitiveType() == sf::Lines ? 2 : 4;
                    for (int i = 0; i < main_vector.size(); i++)
                    {
                        int idx = main_vector[i].second * temp;
                        orig_vertex_vector[idx].texCoords = sf::Vector2f(pixels_per_rect * i, 0);
                        orig_vertex_vector[idx + 1].texCoords = sf::Vector2f(pixels_per_rect * i, sort_image_text.getSize().y);
                        if (temp == 4)
                        {
                            orig_vertex_vector[idx + 2].texCoords = sf::Vector2f(pixels_per_rect * (i + 1), sort_image_text.getSize().y);
                            orig_vertex_vector[idx + 3].texCoords = sf::Vector2f(pixels_per_rect * (i + 1), 0);
                        }
                    }
                }
            }
        }

        // Second sort done with PURE_SORTING set to false, to fill the changes array, get the comparison and write amount.
        for (int i = 0; i < sorts; i++)
        {
            repopulate_vector();
            current_sort = i;
            comparisons[i] = 0;
            writes[i] = 0;

            can_sort = false;
            can_populate = false;

            if (sort_chosen[i] == Sort::bubble) bubble_sort();
            else if (sort_chosen[i] == Sort::coctail_shaker) coctail_shaker_sort();
            else if (sort_chosen[i] == Sort::insertion) insertion_sort();
            else if (sort_chosen[i] == Sort::shell) shell_sort();
            else if (sort_chosen[i] == Sort::quick) quick_sort();
            else if (sort_chosen[i] == Sort::merge) merge_sort();
            else if (sort_chosen[i] == Sort::heap) heap_sort();
            else if (sort_chosen[i] == Sort::counting) counting_sort();
            else if (sort_chosen[i] == Sort::radix) radix_sort();
        }

        // If mode is set to compare, change all rectangles and vertices according to elements in already sorted array.
        // Additionally, push 1 comparison to changes array so the sort ending animation triggers.
        if (!IS_ANIMATION)
        {
            for (int i = 0; i < main_vector.size(); i++)
            {
                if (!vertices_used)
                {
                    rectangle_vector[i]->setSize(sf::Vector2f(element_width, double(main_vector[i].first) / (MAX_ELEMENT * 1.1) * HEIGHT));
                    rectangle_vector[i]->setPosition(sf::Vector2f(i * element_width, HEIGHT - rectangle_vector[i]->getSize().y));
                }
                else
                {
                    int i_idx = (i * (vertex_vector.getPrimitiveType() == sf::Lines ? 2 : 4));
                    vertex_vector[i_idx].position.y = HEIGHT - (double(main_vector[i].first) / (MAX_ELEMENT * 1.1) * HEIGHT);
                    if (vertex_vector.getPrimitiveType() == sf::Quads) vertex_vector[i_idx + 3].position.y = HEIGHT - (double(main_vector[i].first) / (MAX_ELEMENT * 1.1) * HEIGHT);
                }
            }
            changes.push_back({ 0, 0, (int)ActionType::comparison});
        }
    }

    void bubble_sort()
    {
        for (int i = 0; i < main_vector.size(); i++)
        {
            for (int j = 1; j < main_vector.size() - i; j++)
            {
                if (PURE_SORTING)
                {
                    if (main_vector[j - 1].first > main_vector[j].first)
                    {
                        std::swap(main_vector[j - 1], main_vector[j]);
                    }
                }
                else if (compare(j - 1, j, more_than)) swap(j - 1, j);
            }
        }
    }

    void coctail_shaker_sort()
    {
        for (int i = 0; i < main_vector.size() / 2 + 1; i++)
        {
            bool sorted = true;
            for (int j = i + 1; j < main_vector.size() - i; j++)
            {
                if (PURE_SORTING)
                {
                    if (main_vector[j - 1].first > main_vector[j].first)
                    {
                        std::swap(main_vector[j - 1], main_vector[j]);
                        sorted = false;
                    }
                }
                else if (compare(j - 1, j, more_than))
                {
                    swap(j - 1, j);
                    sorted = false;
                }
            }
            if (sorted) break;
            sorted = true;
            for (int j = main_vector.size() - 1 - i; j >= 1 + i; j--)
            {
                if (PURE_SORTING)
                {
                    if (main_vector[j - 1].first > main_vector[j].first)
                    {
                        std::swap(main_vector[j - 1], main_vector[j]);
                        sorted = false;
                    }
                }
                else if (compare(j - 1, j, more_than))
                {
                    swap(j - 1, j);
                    sorted = false;
                }
            }
            if (sorted) break;
        }
    }

    void insertion_sort()
    {
        for (int i = 1; i < main_vector.size(); i++)
        {
            for (int j = i; j > 0; j--)
            {
                if (PURE_SORTING)
                {
                    if (main_vector[j - 1].first > main_vector[j].first)
                    {
                        std::swap(main_vector[j - 1], main_vector[j]);
                    }
                    else break;
                }
                else if (compare(j - 1, j, more_than)) swap(j - 1, j);
                else break;
            }
        }
    }

    void shell_sort()
    {
        for (int gap = main_vector.size() / 2; gap >= 1; gap /= 2)
        {
            for (int i = gap; i < main_vector.size(); i++)
            {
                for (int j = i; j >= gap && (PURE_SORTING ? main_vector[j - gap].first > main_vector[j].first : compare(j - gap, j, more_than)); j -= gap)
                {
                    if (PURE_SORTING)
                    {
                        std::swap(main_vector[j], main_vector[j - gap]);
                    }
                    else swap(j, j - gap);
                }
            }
        }
    }

    void quick_sort()
    {
        quick_sort_helper(0, main_vector.size() - 1);
    }

    void quick_sort_helper(int left, int right)
    {
        if (left >= right) return;
        int mid = (left + right) / 2;
        if (mid != right)
        {
            if (PURE_SORTING)
            {
                std::swap(main_vector[mid], main_vector[right]);
            }
            else swap(mid, right);
        }
        int idx = left;
        for (int i = left; i < right; i++)
        {
            if (PURE_SORTING)
            {
                if (main_vector[i].first < main_vector[right].first)
                {
                    std::swap(main_vector[i], main_vector[idx]);
                    idx++;
                }   
            }
            else if (compare(i, right, less_than))
            {
                swap(i, idx);
                idx++;
            }
        }
        if (idx != right)
        {
            if (PURE_SORTING)
            {
                std::swap(main_vector[idx], main_vector[right]);
            }
            else swap(idx, right);
        }
        int right_idx = idx + 1;
        for (int i = idx + 1; i <= right; i++)
        {
            if (PURE_SORTING)
            {
                if (main_vector[i].first == main_vector[idx].first)
                {
                    std::swap(main_vector[i], main_vector[right_idx]);
                    right_idx++;
                }
            }
            else if (compare(i, idx, [](int a, int b) {return a == b; }))
            {
                swap(i, right_idx);
                right_idx++;
            }
        }
        quick_sort_helper(left, idx - 1);
        quick_sort_helper(right_idx, right);
    }

    void merge_sort()
    {
        merge_sort_helper(0, main_vector.size() - 1);
    }

    void merge_sort_helper(int left, int right)
    {
        if (left >= right) return;
        int mid = (left + right) / 2;
        merge_sort_helper(left, mid);
        merge_sort_helper(mid + 1, right);
        std::vector<std::pair<int, int>> arr(right - left + 1);
        int idx = 0;
        int left_idx = left, right_idx = mid + 1;
        while (left_idx <= mid || right_idx <= right)
        {
            if (IS_ANIMATION)
            {
                if (left_idx > mid) arr[idx++].first = right_idx++;
                else if (right_idx > right) arr[idx++].first = left_idx++;
                else
                {
                    if (PURE_SORTING)
                    {
                        if (main_vector[left_idx].first < main_vector[right_idx].first) arr[idx++].first = left_idx++;
                        else arr[idx++].first = right_idx++;
                    }
                    else
                    {
                        if (compare(left_idx, right_idx, less_than)) arr[idx++].first = left_idx++;
                        else arr[idx++].first = right_idx++;
                    }
                }
            }
            else
            {
                if (left_idx > mid) arr[idx++] = main_vector[right_idx++];
                else if (right_idx > right) arr[idx++] = main_vector[left_idx++];
                else
                {
                    if (PURE_SORTING)
                    {
                        if (main_vector[left_idx].first < main_vector[right_idx].first) arr[idx++] = main_vector[left_idx++];
                        else arr[idx++] = main_vector[right_idx++];
                    }
                    else
                    {
                        if (compare(left_idx, right_idx, less_than)) arr[idx++] = main_vector[left_idx++];
                        else arr[idx++] = main_vector[right_idx++];
                    }
                }
            }
        }

        if (IS_ANIMATION)
        {
            for (int i = left; i <= right; i++)
            {
                swap(i, arr[i - left].first);
                for (int j = i - left + 1; j <= right - left; j++)
                {
                    if (arr[j].first == i)
                    {
                        arr[j] = arr[i - left];
                        break;
                    }
                }
            }
        }
        else
        {
            idx = left;
            if (!IS_ANIMATION) writes[current_sort] += 2 * arr.size();
            for (int i = 0; i < arr.size(); i++) main_vector[idx++] = arr[i];
        }
    }

    void heapify(int idx, int size)
    {
        int largest_idx = idx, left_idx = 2 * idx + 1, right_idx = 2 * idx + 2;
        if (left_idx < size)
        {
            if (PURE_SORTING)
            {
                if (main_vector[left_idx].first > main_vector[largest_idx].first) largest_idx = left_idx;
            }
            else
            {
                if (compare(left_idx, largest_idx, more_than)) largest_idx = left_idx;
            }
        }
        if (right_idx < size)
        {
            if (PURE_SORTING)
            {
                if (main_vector[right_idx].first > main_vector[largest_idx].first) largest_idx = right_idx;
            }
            else
            {
                if (compare(right_idx, largest_idx, more_than)) largest_idx = right_idx;
            }
        }
        if (idx != largest_idx)
        {
            if (PURE_SORTING) std::swap(main_vector[idx], main_vector[largest_idx]);
            else swap(idx, largest_idx);
            heapify(largest_idx, size);
        }
    }
    void heap_sort()
    {
        for (int i = main_vector.size() / 2; i >= 0; i--) heapify(i, main_vector.size());
        for (int i = main_vector.size() - 1; i >= 0; i--)
        {
            if (PURE_SORTING) std::swap(main_vector[0], main_vector[i]);
            else swap(0, i);
            heapify(0, i);
        }
    }


    void counting_sort()
    {
        std::vector<std::pair<int, int>> arr(MAX_ELEMENT + 1, std::pair<int, int>(0, 0));
        writes[current_sort] += MAX_ELEMENT;
        for (int i = 0; i < main_vector.size(); i++)
        {
            arr[main_vector[i].first].first++;
            arr[main_vector[i].first].second = i;
            compare(i, i, [](int a, int b) {return a < b; });
        }
        int idx = 0;
        for (int i = 0; i < arr.size(); i++)
        {
            while (arr[i].first > 0)
            {
                arr[i].first--;
                set(idx++, arr[i].second);
            }
        }
    }

    void radix_sort()
    {
        int max_element = main_vector[0].first, max_element_idx = 0;
        for (int i = 1; i < main_vector.size(); i++)
        {
            if (compare(i, max_element_idx, more_than))
            {
                max_element = main_vector[i].first;
                max_element_idx = i;
            }
        }

        int iterations = int(log10(max_element)) + 1;
        int divisor = 1;

        for (int iteration = 0; iteration < iterations; iteration++)
        {
            std::vector<std::vector<int>> counting_arr(10, std::vector<int>());
            for (int i = 0; i < main_vector.size(); i++)
            {
                int current_digit = (main_vector[i].first / divisor) % 10;
                counting_arr[current_digit].push_back(new_idxs[i]);
            }
            int idx = 0;
            for (int i = 0; i < 10; i++)
            {
                for (int j = 0; j < counting_arr[i].size(); j++)
                {
                    set(idx, counting_arr[i][j]);
                    new_idxs[idx] = counting_arr[i][j];
                    idx++;
                }
            }
            divisor *= 10;
        }
    }

    int MAX_ELEMENT = 1000;
    bool done_phase = false, can_sort = true, can_populate = true, sorted = false, stopped = false, image_sort = false;
    std::vector<int> sort_to_max_limited = { 2000, 2000, 2000, 10000, 10000, 10000, 10000, 20000, 20000 };
    std::vector<int> sort_to_max_unlimited = { 40000, 40000, 40000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000 };
    std::vector<Sort> sort_chosen = { Sort::bubble, Sort::insertion, Sort::quick };
    int current_sort = 0;
private:
    enum class ActionType
    {
        comparison, swap, set, green_set
    };
    bool vertices_used = false;
    bool PURE_SORTING = false;
    sf::RenderWindow* window;

    std::vector<sf::Texture*> texts;
    std::vector<std::pair<int, int>> main_vector;
    std::vector<int> orig_main_vector;
    std::vector<int> orig_idxs;
    std::vector<sf::RectangleShape*> rectangle_vector, orig_rectangle_vector;
    sf::VertexArray vertex_vector, orig_vertex_vector;
    std::vector<int> new_idxs;
    sf::Image sort_image;
    sf::Texture sort_image_text;

    std::vector<std::vector<int>> changes;
    int ACTIONS_DONE = 0;
    double element_width;

    sf::SoundBuffer sound_buffer;
    sf::SoundBuffer trimmed_buffer;
    std::vector<float> beep_pitches;
    std::deque<sf::Sound*> sound_deque;

    std::vector<sf::Text> info_text, stat_text, time_text;
    sf::Font font;
    std::vector<int> comparisons = { 0, 0, 0 }, writes = {0, 0, 0};
    std::vector<std::string> sort_info = { "Bubble Sort",
                                            "Shaker Sort",
                                            "Insertion Sort",
                                            "Shell Sort",
                                            "Quick Sort",
                                            "Merge Sort",
                                            "Heap Sort",
                                            "Counting Sort",
                                            "Radix Sort" };
};

// Button class, with many parameters controlling its behaviour
class Button
{
public:
    Button() {}
    Button(float x_, float y_, float width_, float height_, std::string text_, sf::RenderWindow* window_, MainVector* main_vec_,
        bool push_to_vector, bool holdable_ = true, Button* parent_ = nullptr, bool unpress_on_press_ = false, bool is_pressable_when_sorting_ = true, int priority_ = 0, bool enabled_ = true, 
        sf::Color NEW_NORMAL_COLOR = sf::Color::Transparent, sf::Color NEW_HOVERED_COLOR = sf::Color::Transparent, sf::Color NEW_PRESSED_COLOR = sf::Color::Transparent) : x(x_), y(y_), width(width_), height(height_), holdable(holdable_), text_string(text_), start_text(text_)
    {
        if (push_to_vector) all_buttons.push_back(ButtonPriority(Pressable::button, (void*)this, priority_));
        if (NEW_NORMAL_COLOR != sf::Color::Transparent)
        {
            NORMAL_BUTTON_COLOR = NEW_NORMAL_COLOR;
            HOVERED_BUTTON_COLOR = NEW_HOVERED_COLOR;
            PRESSED_BUTTON_COLOR = NEW_PRESSED_COLOR;
        }
        rect.setSize(sf::Vector2f(width, height));
        rect.setPosition(sf::Vector2f(x, y));
        rect.setFillColor(NORMAL_BUTTON_COLOR);
        rect.setOutlineThickness(3);
        rect.setOutlineColor(sf::Color(100, 100, 100));
        font.loadFromFile("regular.ttf");

        text.setFont(font);
        text.setCharacterSize(26);
        text.setFillColor(NORMAL_BUTTON_COLOR == sf::Color(30, 30, 30) ? sf::Color(240, 240, 240) : sf::Color(30, 30, 30));
        text.setString(text_string);
        sf::FloatRect text_rect = text.getLocalBounds();
        while (text_rect.getSize().x > width_ * 0.8 or text_rect.getSize().y > height_ * 0.7)
        {
            text.setCharacterSize(text.getCharacterSize() - 1);
            text_rect = text.getLocalBounds();
        }
        text.setOrigin(text_rect.getSize().x / 2, text_rect.getSize().y / 2);
        text.setPosition(sf::Vector2f(x + width / 2, y + height / 2 - 7));

        main_vec = main_vec_;
        window = window_;
        parent = parent_;
        unpress_on_press = unpress_on_press_;
        is_pressable_when_sorting = is_pressable_when_sorting_;
        enabled = enabled_;
    }
    bool is_pressed() const
    {
        return pressed;
    }
    bool press()
    {
        if (!is_pressable_when_sorting and !main_vec->can_sort) return false;
        pressed = true;
        rect.setFillColor(PRESSED_BUTTON_COLOR);

        for (int i = 0; i < string_to_sort.size(); i++)
        {
            if (!holdable && string_to_sort[i].first == text_string)
            {
                int sort_number = parent->start_text == "BUBBLE" ? 0 : (parent->start_text == "INSERTION" ? 1 : 2);
                main_vec->set_sort(string_to_sort[i].second, sort_number);
                parent->set_string(text_string);
                break;
            }
        }
        if (text_string == "START" && main_vec->can_sort) main_vec->sort();
        if (text_string == "STOP")
        {
            if (main_vec->stopped) main_vec->unstop();
            else main_vec->stop();
        }
        if (!holdable) unpress();
        return true;
    }

    bool unpress()
    {
        pressed = false;
        rect.setFillColor(HOVERED_BUTTON_COLOR);
        return true;
    }

    void set_string(std::string new_string)
    {
        text_string = new_string;
        text.setCharacterSize(26);
        text.setString(text_string);
        sf::FloatRect text_rect = text.getLocalBounds();
        while (text_rect.getSize().x > width * 0.8)
        {
            text.setCharacterSize(text.getCharacterSize() - 1);
            text_rect = text.getLocalBounds();
        }
        text.setOrigin(text_rect.getSize().x / 2, text_rect.getSize().y / 2);
        text.setPosition(sf::Vector2f(x + width / 2, y + height / 2 - 7));
    }

    bool update(sf::Vector2i mouse_pos, bool mouse_pressed)
    {
        if (!enabled) return false;
        if (rect.getGlobalBounds().contains(sf::Vector2f(mouse_pos.x, mouse_pos.y)))
        {
            hovered = true;
            if (rect.getFillColor() == NORMAL_BUTTON_COLOR) rect.setFillColor(HOVERED_BUTTON_COLOR);
            if (mouse_pressed)
            {
                if (pressed) return unpress();
                else return press();
            }
        }
        else
        {
            if (mouse_pressed && unpress_on_press) unpress();
            hovered = false;
            if (rect.getFillColor() == HOVERED_BUTTON_COLOR) rect.setFillColor(NORMAL_BUTTON_COLOR);
        }
        return false;
    }

    void draw()
    {
        if (!enabled) return;
        window->draw(rect);
        window->draw(text);
    }

    bool enabled = true;
protected:
    float x = 0, y = 0;
    float width = 0, height = 0;
    sf::RectangleShape rect;

    sf::Color NORMAL_BUTTON_COLOR = sf::Color(30, 30, 30);
    sf::Color HOVERED_BUTTON_COLOR = sf::Color(60, 60, 60);
    sf::Color PRESSED_BUTTON_COLOR = sf::Color(90, 90, 90);

    sf::Font font;
    sf::Text text;
    std::string text_string;
    std::string start_text;

    bool hovered = false, pressed = false, holdable = true, unpress_on_press = false;
    bool is_pressable_when_sorting = true;

    sf::RenderWindow* window = nullptr;
    MainVector* main_vec = nullptr;
    Button* parent = nullptr;

    std::vector<std::pair<std::string, Sort>> string_to_sort = { {"BUBBLE", Sort::bubble},
                                                                {"SHAKER", Sort::coctail_shaker},
                                                                {"INSERTION", Sort::insertion},
                                                                {"SHELL", Sort::shell},
                                                                {"QUICK", Sort::quick},
                                                                {"MERGE", Sort::merge},
                                                                {"HEAP", Sort::heap},
                                                                {"COUNTING", Sort::counting},
                                                                { "RADIX", Sort::radix } };
};


// DropDownButton is a button with multiple button choices
class DropDownButton
{
public:
    DropDownButton(float x_, float y_, float width_, float height_, std::string text_, std::vector<std::string> choices_, sf::RenderWindow* window_, MainVector* main_vec_)
    {
        all_buttons.push_back(ButtonPriority(Pressable::dropdown, (void*)this, 0));
        head = new Button(x_, y_, width_, height_, text_, window_, main_vec_, false, true, nullptr, true, false);
        float current_x = x_, current_y = y_ + height_;
        choices.resize(choices_.size(), nullptr);
        for (int i = 0; i < choices.size(); i++)
        {
            choices[i] = new Button(current_x, current_y, width_, height_ * 0.8, choices_[i], window_, main_vec_, false, false, head, false, false, 1, false);
            current_y += height_ * 0.8;
        }
    }
    bool update(sf::Vector2i mouse_pos, bool mouse_pressed)
    {
        if (!head->enabled) return false;
        bool ans = false;
        if (head->is_pressed())
        {
            for (int i = 0; i < choices.size(); i++)
            {
                choices[i]->enabled = true;
                if (choices[i]->update(mouse_pos, mouse_pressed))
                {
                    ans = true;
                    break;
                }
            }
        }
        if (head->update(mouse_pos, mouse_pressed)) ans = true;
        return ans;
    }
    void draw()
    {
        if (!head->enabled) return;
        head->draw();
        if (head->is_pressed())
        {
            for (int i = 0; i < choices.size(); i++)
                choices[i]->draw();
        }
    }
    void enable()
    {
        head->enabled = true;
    }
    void disable()
    {
        head->enabled = false;
        head->unpress();
    }
private:
    Button* head = nullptr;
    std::vector<Button*> choices;
};

// Slider class, used for changing variables in set range
class Slider
{
public:
    Slider(float x_, float y_, float width_, float height_, std::string text_, std::pair<int, int> range_, int* changing_value_, sf::RenderWindow* window_, MainVector* main_vec_, std::pair<int, int> max_range_ = { -1, -1 }) : x(x_), y(y_), width(width_), height(height_), text_string(text_), range(range_), max_range(max_range_)
    {
        all_buttons.push_back(ButtonPriority(Pressable::slider, (void*)this, 0));
        window = window_;
        main_vec = main_vec_;
        changing_value = changing_value_;

        slider_width = width * 0.8;

        line.setSize(sf::Vector2f(slider_width + 1, 1));
        line.setPosition(sf::Vector2f(x, y + (height / 2)));
        line.setFillColor(sf::Color(150, 150, 230));

        if (max_range.second != -1)
        {
            max_line.setSize(sf::Vector2f(2, height / 3));
            max_line.setPosition(sf::Vector2f(x + slider_width * MAX_RANGE_COEFF, y + 14));
            max_line.setFillColor(sf::Color(255, 130, 130));
        }

        rect.setSize(sf::Vector2f(slider_width + 1, height));
        rect.setPosition(sf::Vector2f(x, y));

        moving_rect.setSize(sf::Vector2f(slider_width / 14, height / 2));
        moving_rect.setFillColor(sf::Color(30, 30, 30));
        moving_rect.setOrigin(sf::Vector2f(slider_width / 28, height / 4));
        moving_rect.setPosition(sf::Vector2f(x + slider_width / 2, y + height / 2));


        font.loadFromFile("regular.ttf");
        text.setFont(font);
        text.setCharacterSize(16);
        text.setFillColor(sf::Color(30, 30, 30));
        text.setString(text_string);
        sf::FloatRect text_rect = text.getLocalBounds();
        text.setOrigin(text_rect.getSize().x / 2, text_rect.getSize().y);
        text.setPosition(sf::Vector2f(x + slider_width / 2, y));

        value_text.setFont(font);
        value_text.setCharacterSize(16);
        value_text.setFillColor(sf::Color(30, 30, 30));
        text_rect = value_text.getLocalBounds();
        value_text.setOrigin(0, text_rect.getSize().y / 2);
        value_text.setPosition(sf::Vector2f(x + slider_width + 6, y + height / 2 - 10));
    }
    bool update(sf::Vector2i mouse_pos, bool mouse_pressed)
    {
        if (!mouse_pressed) return false;
        if (rect.getGlobalBounds().contains(sf::Vector2f(mouse_pos.x, mouse_pos.y)))
        {
            double ratio = (mouse_pos.x - x) / slider_width;
            moving_rect.setPosition(sf::Vector2f(x + slider_width * ratio, y + height / 2));
            int new_value = (ratio >= MAX_RANGE_COEFF && max_range.second != -1) ? (max_range.first + (((ratio - MAX_RANGE_COEFF) / (1 - MAX_RANGE_COEFF)) * (max_range.second - max_range.first))) : range.first + int((range.second - range.first) * (ratio / (max_range.second == -1 ? 1 : MAX_RANGE_COEFF)));
            *changing_value = new_value;
            value_text.setString(std::to_string(*changing_value));
            return true;
        }
        return false;
    }
    void set(int val)
    {
        if (val < range.first || val > range.second) return;
        double ratio = double(val - range.first) / (range.second - range.first + 1);
        moving_rect.setPosition(sf::Vector2f(x + slider_width * ratio, y + height / 2));
        *changing_value = val;
        value_text.setString(std::to_string(*changing_value));
    }
    void draw()
    {
        window->draw(value_text);
        window->draw(text);
        window->draw(line);
        if (max_range.second != -1) window->draw(max_line);
        window->draw(moving_rect);
    }
private:
    float x, y, width, height;
    float slider_width;
    std::pair<int, int> range, max_range;
    int* changing_value;
    const double MAX_RANGE_COEFF = 0.8;

    sf::Font font;
    sf::Text text, value_text;
    std::string text_string;

    sf::RectangleShape line, rect, moving_rect, max_line;
    sf::RenderWindow* window = nullptr;
    MainVector* main_vec = nullptr;
};

// TextWindow class, used for changing variables with text entry boxes
class TextWindow
{
public:
    TextWindow(float x_, float y_, float width_, float height_, std::string name_, std::vector<std::string> texts, std::vector<std::pair<int, int>> ranges_, std::vector<int> values_, sf::RenderWindow* window_, MainVector* main_vec_, int button_amount = 1) : x(x_), y(y_), width(width_), height(height_)
    {
        all_buttons.push_back(ButtonPriority(Pressable::textbox, (void*)this, 0));
        name = name_;
        font.loadFromFile("regular.ttf");

        window = window_;
        main_vec = main_vec_;
        ranges = ranges_;
        values = values_;
        set_button = new Button(x, y + height + 2, width, height / 2, "SET", window, main_vec, false, false);

        main_buttons.resize(button_amount, nullptr);
        for (int i = 0; i < button_amount; i++)
        {
            main_buttons[i] = new Button(x + (i * width / button_amount), y, width / button_amount, height, std::to_string(values[i]), window, main_vec, false, true, nullptr, true, true, 0, true, sf::Color(220, 220, 220), sf::Color(190, 190, 190), sf::Color(160, 160, 160));
        }

        info_text.resize(button_amount);
        for (int i = 0; i < button_amount; i++)
        {
            info_text[i].setFont(font);
            info_text[i].setString(texts[i]);
            info_text[i].setCharacterSize(16);
            info_text[i].setFillColor(sf::Color(30, 30, 30));
            sf::FloatRect text_rect = info_text[i].getLocalBounds();
            info_text[i].setOrigin(text_rect.getSize().x / 2, text_rect.getSize().y);
            info_text[i].setPosition(sf::Vector2f(x + (i * width / button_amount) + (width / button_amount) / 2, y - 15));
        }
    }

    bool update(sf::Vector2i mouse_pos, bool mouse_pressed, sf::Keyboard::Scan::Scancode key_pressed = sf::Keyboard::Scan::Unknown)
    {
        bool ans = false;
        for (int i = 0; i < main_buttons.size(); i++)
        {
            if (main_buttons[i]->update(mouse_pos, mouse_pressed)) ans = true;
        }

        if (set_button->update(mouse_pos, mouse_pressed))
        {
            ans = true;
            if (name == "SETTINGS")
            {
                POPULATION = std::max(ranges[0].first, values[0]);
                main_vec->MAX_ELEMENT = std::max(ranges[1].first, values[1]);
                main_vec->populate_vector(POPULATION);
            }
        }

        for (int i = 0; i < main_buttons.size(); i++)
        {
            if (main_buttons[i]->is_pressed())
            {
                int num_pressed = key_pressed - sf::Keyboard::Scan::Num1;
                if (num_pressed >= 0 && num_pressed <= 9)
                {
                    num_pressed = (num_pressed + 1) % 10;
                    values[i] = values[i] * 10 + num_pressed;
                }
                else if (key_pressed == sf::Keyboard::Scan::Backspace)
                {
                    values[i] /= 10;
                }
                main_buttons[i]->set_string(std::to_string(values[i]));
            }
            if (name == "SETTINGS")
            {
                if (IS_ANIMATION)
                {
                    if (values[0] > main_vec->sort_to_max_limited[int(main_vec->sort_chosen[0])]) values[0] = main_vec->sort_to_max_limited[int(main_vec->sort_chosen[0])];
                }
                else
                {
                    int min_max_size = std::min({ main_vec->sort_to_max_unlimited[int(main_vec->sort_chosen[0])], main_vec->sort_to_max_unlimited[int(main_vec->sort_chosen[1])] , main_vec->sort_to_max_unlimited[int(main_vec->sort_chosen[2])] });
                    if (values[0] > min_max_size) values[0] = min_max_size;
                }
                if (values[1] > ranges[1].second) values[1] = ranges[1].second;
            }
            main_buttons[i]->set_string(std::to_string(values[i]));
        }
        return ans;
    }

    void draw()
    {
        for (int i = 0; i < main_buttons.size(); i++)
        {
            main_buttons[i]->draw();
            window->draw(info_text[i]);
        }
        set_button->draw();
    }
protected:
    float x, y, width, height;

    sf::Font font;
    sf::RenderWindow* window;
    MainVector* main_vec;

    sf::Color NORMAL_BUTTON_COLOR = sf::Color(255, 255, 255);
    sf::Color HOVERED_BUTTON_COLOR = sf::Color(220, 220, 220);
    sf::Color PRESSED_BUTTON_COLOR = sf::Color(190, 190, 190);

    std::string name;
    std::vector<int> values = {};
    std::vector<sf::Text> info_text;
    std::vector<std::pair<int, int>> ranges;
    Button* set_button = nullptr;
    std::vector<Button*> main_buttons;
};

void func(MainVector* vec)
{
    vec->populate_vector(vec->get_size());
}

// CheckBox class, used for changing variables that have only 2 states
class CheckBox
{
public:
    CheckBox(float x, float y, float width, std::vector<std::string> text_, bool* changing_value_, std::pair<int, int> set_values_, bool default_state, bool enabled_, sf::RenderWindow* window_, MainVector* main_vec_, void (*func)(MainVector*))
    {
        enabled = enabled_;
        press_f = func;
        all_buttons.push_back(ButtonPriority(Pressable::checkbox, (void*)this, 0));
        main_vec = main_vec_;
        set_values = set_values_;
        changing_value = changing_value_;
        window = window_;
        text_vector = text_;

        check_button = new Button(x, y, width, width, "", window_, main_vec_, false, true, nullptr, false, true, 0, true, sf::Color(240, 240, 240), sf::Color(200, 255, 200), sf::Color(30, 255, 30));
        if (default_state)
        {
            check_button->press();
            *changing_value = set_values_.second;
        }
        font.loadFromFile("regular.ttf");
        text.setFont(font);
        text.setString(text_vector[int(default_state)]);
        text.setCharacterSize(16);
        text.setFillColor(sf::Color(30, 30, 30));
        sf::FloatRect text_rect = text.getLocalBounds();
        text.setOrigin(0, text_rect.getSize().y / 2);
        text.setPosition(sf::Vector2f(SORT_WIDTH + 10, y + width / 2 - 5));
    }
    void draw()
    {
        if (text_vector[0] == "RECT MODE" && !IS_ANIMATION) return;
        window->draw(text);
        check_button->draw();
    }
    bool update(sf::Vector2i mouse_pos, bool mouse_pressed)
    {
        if (text_vector[0] == "RECT MODE" && !IS_ANIMATION && check_button->is_pressed())
        {
            check_button->unpress();
            *changing_value = set_values.first;
            text.setString(text_vector[0]);
            if (press_f != nullptr) press_f(main_vec);
            return false;
        }
        if (!main_vec->can_sort) return false;
        bool ans = check_button->update(mouse_pos, mouse_pressed);
        if (ans)
        {
            if (check_button->is_pressed())
            {
                *changing_value = set_values.second;
                text.setString(text_vector[1]);
            }
            else
            {
                *changing_value = set_values.first;
                text.setString(text_vector[0]);
            }
            if (press_f != nullptr) press_f(main_vec);
        }
        return ans;
    }
    bool enabled;
private:
    sf::RenderWindow* window;
    MainVector* main_vec;
    sf::Font font;
    sf::Text text;
    Button* check_button;
    bool* changing_value;
    std::pair<int, int> set_values;
    std::vector<std::string> text_vector;
    void(*press_f)(MainVector*);
};

int main()
{
    srand(time(nullptr));

    // Create main window and set its parameters
    sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "Sorting", sf::Style::Titlebar | sf::Style::Close);
    sf::Image icon;
    icon.loadFromFile("icon.png");
    window.setFramerateLimit(60);
    //window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());

    sf::Image sort_image;
    sort_image.loadFromFile("image.jpeg");

    // Rectangle of the menu, buttons are placed based on its size and position
    sf::RectangleShape menu_rect(sf::Vector2f(WIDTH - SORT_WIDTH, HEIGHT));
    menu_rect.setPosition(sf::Vector2f(SORT_WIDTH + 4, 0));
    menu_rect.setFillColor(sf::Color(220, 220, 220));
    menu_rect.setOutlineThickness(4);
    menu_rect.setOutlineColor(sf::Color(100, 100, 100));

    POPULATION = 100;
    MainVector vec(POPULATION, &window);

    // Create all the main buttons
    Button start_button(SORT_WIDTH + 12, HEIGHT - 70, (WIDTH - SORT_WIDTH) - 20, 60, "START", &window, &vec, true, false);
    Button stop_button(SORT_WIDTH + 12, HEIGHT - 110, (WIDTH - SORT_WIDTH) - 20, 30, "STOP", &window, &vec, true, false);

    DropDownButton sort_choice_button1(SORT_WIDTH + 12, 10, (WIDTH - SORT_WIDTH) - 20, 50, "BUBBLE", { "BUBBLE", "SHAKER", "INSERTION", "SHELL", "QUICK", "MERGE", "HEAP", "COUNTING", "RADIX" }, &window, &vec);
    DropDownButton sort_choice_button2(SORT_WIDTH + 12, 70, (WIDTH - SORT_WIDTH) - 20, 50, "INSERTION", { "BUBBLE", "SHAKER", "INSERTION", "SHELL", "QUICK", "MERGE", "HEAP", "COUNTING", "RADIX" }, &window, &vec);
    DropDownButton sort_choice_button3(SORT_WIDTH + 12, 130, (WIDTH - SORT_WIDTH) - 20, 50, "QUICK", { "BUBBLE", "SHAKER", "INSERTION", "SHELL", "QUICK", "MERGE", "HEAP", "COUNTING", "RADIX" }, &window, &vec);

    Slider animation_speed_slider(SORT_WIDTH + 12, HEIGHT - 260, (WIDTH - SORT_WIDTH) - 20, 40, "ANIM. SPEED", { 1, 100 }, &ACTIONS_PER_FRAME, &window, &vec, { 100, 1000 });
    Slider sound_slider(SORT_WIDTH + 12, HEIGHT - 320, (WIDTH - SORT_WIDTH) - 20, 40, "SOUND", { 0, 100 }, &SOUND_COEFF, &window, &vec);
    animation_speed_slider.set(ACTIONS_PER_FRAME);
    sound_slider.set(SOUND_COEFF);

    CheckBox animate_checkbox(WIDTH - 30, HEIGHT - 370, 20, { "COMPARE MODE", "ANIMATE MODE"}, &IS_ANIMATION, {0, 1}, true, true, &window, &vec, nullptr);
    CheckBox image_checkbox(WIDTH - 30, HEIGHT - 400, 20, { "RECT MODE", "IMAGE MODE" }, &vec.image_sort, { 0, 1 }, false, true, &window, &vec, func);

    TextWindow population_window(SORT_WIDTH + 12, HEIGHT - 190, (WIDTH - SORT_WIDTH) - 20, 40, "SETTINGS", { "SIZE", "MAX" }, { { 5, 2000 }, {2, 100000000} }, { POPULATION, vec.MAX_ELEMENT }, &window, &vec, 2);

    // Sort them based on their priority
    sort(all_buttons.begin(), all_buttons.end(), [](ButtonPriority& left, ButtonPriority& right) {return left.priority > right.priority; });

    while (window.isOpen())
    {
        if (IS_ANIMATION)
        {
            sort_choice_button2.disable();
            sort_choice_button3.disable();
        }
        else
        {
            sort_choice_button2.enable();
            sort_choice_button3.enable();
        }
        sf::Event event;
        bool mouse_pressed_this_frame = false, mouse_pressed = false;
        sf::Keyboard::Scan::Scancode key_pressed = sf::Keyboard::Scan::Unknown;

        // Handle events loop
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            else if (event.type == sf::Event::MouseButtonPressed)
            {
                if (event.mouseButton.button == sf::Mouse::Left) mouse_pressed_this_frame = true;
            }
            else if (event.type == sf::Event::KeyPressed)
            {
                key_pressed = event.key.scancode;
            }
        }
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) mouse_pressed = true;

        sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);

        // Update all the buttons
        // Looks very bad, unfortunately this is the least problematic way of handling button priority without rewriting it from the ground up.
        for (int i = 0; i < all_buttons.size(); i++)
        {
            if (all_buttons[i].pressable == Pressable::button)
            {
                Button* temp_ptr = (Button*)all_buttons[i].button_ptr;
                if (temp_ptr->update(mouse_pos, mouse_pressed_this_frame)) break;
            }
            else if (all_buttons[i].pressable == Pressable::slider)
            {
                Slider* temp_ptr = (Slider*)all_buttons[i].button_ptr;
                if (temp_ptr->update(mouse_pos, mouse_pressed)) break;
            }
            else if (all_buttons[i].pressable == Pressable::dropdown)
            {
                DropDownButton* temp_ptr = (DropDownButton*)all_buttons[i].button_ptr;
                if (temp_ptr->update(mouse_pos, mouse_pressed_this_frame)) break;
            }
            else if (all_buttons[i].pressable == Pressable::checkbox)
            {
                CheckBox* temp_ptr = (CheckBox*)all_buttons[i].button_ptr;
                if (temp_ptr->update(mouse_pos, mouse_pressed_this_frame)) break;
            }
            else if (all_buttons[i].pressable == Pressable::textbox)
            {
                TextWindow* temp_ptr = (TextWindow*)all_buttons[i].button_ptr;
                if (temp_ptr->update(mouse_pos, mouse_pressed_this_frame, key_pressed)) break;
            }
        }

        // Clear the window, draw the menu base and MainVector on it
        window.clear(BACKGROUND_COLOR);
        window.draw(menu_rect);
        vec.draw();
        
        // Draw all the buttons on the window
        for (int i = all_buttons.size() - 1; i >= 0; i--)
        {
            if (all_buttons[i].pressable == Pressable::button)
            {
                Button* temp_ptr = (Button*)all_buttons[i].button_ptr;
                temp_ptr->draw();
            }
            else if (all_buttons[i].pressable == Pressable::slider)
            {
                Slider* temp_ptr = (Slider*)all_buttons[i].button_ptr;
                temp_ptr->draw();
            }
            else if (all_buttons[i].pressable == Pressable::dropdown)
            {
                DropDownButton* temp_ptr = (DropDownButton*)all_buttons[i].button_ptr;
                temp_ptr->draw();
            }
            else if (all_buttons[i].pressable == Pressable::checkbox)
            {
                CheckBox* temp_ptr = (CheckBox*)all_buttons[i].button_ptr;
                temp_ptr->draw();
            }
            else if (all_buttons[i].pressable == Pressable::textbox)
            {
                TextWindow* temp_ptr = (TextWindow*)all_buttons[i].button_ptr;
                temp_ptr->draw();
            }
        }

        // Display changes on the window
        window.display();
    }
}
