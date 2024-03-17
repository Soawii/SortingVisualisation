#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <ctime>
#include <string>
#include <climits>
#include <deque>

int WIDTH = 1200, HEIGHT = 800;
int SORT_WIDTH = WIDTH * 0.85;

sf::Color BACKGROUND_COLOR(230, 230, 230);
sf::Color ACTION_COLOR(255, 0, 0);
sf::Color NORMAL_COLOR(200, 200, 255);
sf::Color DONE_COLOR(0, 255, 0);
int ACTIONS_PER_FRAME = 20;
int SOUND_COEFF = 20;
int POPULATION = 100;
int IS_ANIMATION = 0;

enum class Sort { bubble, coctail_shaker, insertion, quick, merge, heap, counting, radix };

class MainVector
{
public:
    MainVector(size_t population, sf::RenderWindow* new_window)
    {
        font.loadFromFile("regular.ttf");
        window = new_window;
        populate_vector(population);

        info_text.setFont(font);
        info_text.setCharacterSize(16);
        info_text.setFillColor(sf::Color(30, 30, 30));
        info_text.setPosition(10, 10);
        info_text.setString(sort_info[int(sort_chosen)]);

        stat_text.setFont(font);
        stat_text.setCharacterSize(16);
        stat_text.setFillColor(sf::Color(30, 30, 30));
        stat_text.setPosition(10, 10 + 18);

        time_text.setFont(font);
        time_text.setCharacterSize(16);
        time_text.setFillColor(sf::Color(30, 30, 30));
        time_text.setPosition(20 + info_text.getLocalBounds().left + info_text.getLocalBounds().width, 10);
        time_text.setString("Time: ");

        sound_buffer.loadFromFile("laser.wav");

        std::size_t start_cut = sound_buffer.getSampleCount() * 0.46, end_cut = sound_buffer.getSampleCount() * 0.5;
        trimmed_buffer.loadFromSamples(sound_buffer.getSamples() + start_cut, sound_buffer.getSampleCount() - start_cut - end_cut, sound_buffer.getChannelCount(), sound_buffer.getSampleRate());


        beep_pitches.resize(1000, 0);
        for (int i = 0; i < 1000; i++)
        {
            beep_pitches[i] = 0.2 + i * 0.0012;
        }
    }

    void populate_vector(size_t population)
    {
        if (!(can_populate || stopped)) return;
        size = population;
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
            orig_main_vector[i] = MAX_ELEMENT > 1000 ? (int((double(std::rand()) / RAND_MAX) * (MAX_ELEMENT - 1)) + 1) : (std::rand() % MAX_ELEMENT + 1);

            if (!vertices_used)
            {
                orig_rectangle_vector[i] = new sf::RectangleShape();
                orig_rectangle_vector[i]->setSize(sf::Vector2f(element_width - 1, double(orig_main_vector[i]) / (MAX_ELEMENT * 1.1) * HEIGHT));
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

                orig_vertex_vector[i * add] = sf::Vertex(sf::Vector2f(i * element_width, HEIGHT - (double(orig_main_vector[i]) / (MAX_ELEMENT * 1.1) * HEIGHT)), NORMAL_COLOR);
                orig_vertex_vector[i * add + 1] = sf::Vertex(sf::Vector2f(i * element_width, HEIGHT), NORMAL_COLOR);
                vertex_vector[i * add] = orig_vertex_vector[i * add];
                vertex_vector[i * add + 1] = orig_vertex_vector[i * add + 1];
                if (orig_vertex_vector.getPrimitiveType() == sf::Quads)
                {
                    orig_vertex_vector[i * add + 2] = sf::Vertex(sf::Vector2f((i + 1) * element_width, HEIGHT), NORMAL_COLOR);
                    orig_vertex_vector[i * add + 3] = sf::Vertex(sf::Vector2f((i + 1) * element_width, HEIGHT - (double(orig_main_vector[i]) / (MAX_ELEMENT * 1.1) * HEIGHT)), NORMAL_COLOR);
                    vertex_vector[i * add + 2] = orig_vertex_vector[i * add + 2];
                    vertex_vector[i * add + 3] = orig_vertex_vector[i * add + 3];
                }
            }
        }
        main_vector = orig_main_vector;

        can_populate = true;
        can_sort = true;
        sorted = false;
        done_phase = false;
        changes.clear();
        ACTIONS_DONE = 0;
        unstop();
    }

    void repopulate_vector()
    {
        int add = vertex_vector.getPrimitiveType() == sf::Lines ? 2 : 4;
        for (int i = 0; i < main_vector.size(); i++)
        {
            main_vector[i] = orig_main_vector[i];
            if (vertices_used)
            {
                for (int k = 0; k < add; k++)
                {
                    vertex_vector[i * add + k] = orig_vertex_vector[i * add + k];
                }
            }
            else *rectangle_vector[i] = *orig_rectangle_vector[i];
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

    void set_sort(Sort new_sort)
    {
        if (!((can_populate && can_sort) || stopped)) return;
        sort_chosen = new_sort;
        info_text.setString(sort_info[int(sort_chosen)]);
        time_text.setPosition(20 + info_text.getLocalBounds().left + info_text.getLocalBounds().width, 10);
    }

    Sort get_sort() const
    {
        return sort_chosen;
    }

    int get_size() const
    {
        return main_vector.size();
    }

    void swap(int i, int j, bool true_sort)
    {
        if (!IS_ANIMATION) writes += 2;
        int temp1 = main_vector[i];
        main_vector[i] = main_vector[j];
        main_vector[j] = temp1;
        if (!true_sort) changes.push_back({ i, j, 1 });
    }

    void good_swap(int i, int j, bool true_sort)
    {
        if (!IS_ANIMATION) writes += 2;
        swap(i, new_idxs[j], true_sort);
        for (int k = 0; k < main_vector.size(); k++)
        {
            if (new_idxs[k] == i)
            {
                new_idxs[k] = new_idxs[j];
                new_idxs[j] = i;
                break;
            }
        }
    }

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

            if (add == 4)
            {
                temp = vertex_vector[idx_i + 3].position;
                vertex_vector[idx_i + 3].position.y = vertex_vector[idx_j + 3].position.y;
                vertex_vector[idx_j + 3].position.y = temp.y;
            }
        }
    }

    void set(int source, int value, bool true_sort)
    {
        main_vector[source] = orig_main_vector[value];
        if (!IS_ANIMATION) writes++;
        if (!true_sort) changes.push_back({ source, value, 2 });
    }

    void set_value(int idx, int value, bool true_sort)
    {
        main_vector[idx] = value;
        if (!IS_ANIMATION) writes++;
        if (!true_sort) changes.push_back({ idx, idx, 2 });
    }

    bool compare(int i, int j, bool (*cmp)(int, int), bool true_sort)
    {
        if (!true_sort) changes.push_back({ i, j, 0 });
        if (!IS_ANIMATION) comparisons++;
        return cmp(main_vector[i], main_vector[j]);
    }

    bool is_stopped() const
    {
        return stopped;
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

    void draw()
    {
        stat_text.setString("Comparisons: " + std::to_string(comparisons) + "   Writes: " + std::to_string(writes));
        window->draw(stat_text);
        window->draw(info_text);
        window->draw(time_text);

        int min_sound = 1000000000, max_sound = -1;
        double avg = 0;
        int cnt = 0;

        if (ACTIONS_DONE >= changes.size())
        {
            if (changes.size() > 0)
            {
                changes.clear();
                if (!done_phase)
                {
                    for (int i = 0; i < size; i++)
                    {
                        changes.push_back({ i, i, 3 });
                    }
                    done_phase = true;
                }
                else
                {
                    sorted = true;
                    can_sort = true;
                    can_populate = true;
                }
            }
            ACTIONS_DONE = 0;
        }

        int ACTIONS_TO_DO = -1;
        if (ACTIONS_DONE < changes.size())
        {
            ACTIONS_TO_DO = done_phase ? std::max(1, size / 50) : (stopped ? 0 : ACTIONS_PER_FRAME);
            for (int idx = ACTIONS_DONE; idx < std::min(int(changes.size()), ACTIONS_DONE + ACTIONS_TO_DO); idx++)
            {
                int i = changes[idx][0], j = changes[idx][1];
                int add = vertex_vector.getPrimitiveType() == sf::Lines ? 2 : 4;
                int i_idx = (i * add), j_idx = (j * add);
                if (changes[idx][2] == 0)
                {
                    comparisons++;
                }
                if (changes[idx][2] == 1)
                {
                    writes += 2;
                    swap_rectangles(i, j);
                }
                if (changes[idx][2] == 2)
                {
                    writes++;
                    if (vertices_used)
                    {
                        if (vertex_vector.getPrimitiveType() == sf::Lines)
                        {
                            vertex_vector[i_idx].position.y = orig_vertex_vector[j_idx].position.y;
                        }
                        else
                        {
                            vertex_vector[i_idx].position.y = orig_vertex_vector[j_idx].position.y;
                            vertex_vector[i_idx + 3].position.y = orig_vertex_vector[j_idx + 3].position.y;
                        }
                    }
                    else
                    {
                        *(rectangle_vector[i]) = *(orig_rectangle_vector[j]);
                        rectangle_vector[i]->setPosition(sf::Vector2f(i * element_width, HEIGHT - rectangle_vector[i]->getSize().y));
                    }
                }
                if (changes[idx][2] == 3)
                {
                    if (!vertices_used) rectangle_vector[i]->setFillColor(DONE_COLOR);
                    else
                    {
                        for (int k = 0; k < add; k++)
                        {
                            vertex_vector[i_idx + k].color = DONE_COLOR;
                        }
                    }
                }
                else
                {
                    if (!vertices_used) rectangle_vector[i]->setFillColor(ACTION_COLOR);
                    else
                    {
                        for (int k = 0; k < add; k++)
                        {
                            vertex_vector[i_idx + k].color = ACTION_COLOR;
                        }
                    }
                    if (changes[idx][2] != 2)
                    {
                        if (!vertices_used) rectangle_vector[j]->setFillColor(ACTION_COLOR);
                        else
                        {
                            for (int k = 0; k < add; k++)
                            {
                                vertex_vector[j_idx + k].color = ACTION_COLOR;
                            }
                        }
                    }
                }
                if (!vertices_used)
                {
                    avg += rectangle_vector[i]->getSize().y;
                    cnt++;
                    if (i != j)
                    {
                        avg += rectangle_vector[j]->getSize().y;
                        cnt++;
                    }
                }
                else
                {
                    avg += HEIGHT - vertex_vector[i_idx].position.y;
                    cnt++;
                    if (i != j)
                    {
                        avg += HEIGHT - vertex_vector[j_idx].position.y;
                        cnt++;
                    }
                }
            }
        }
        if (!vertices_used)
        {
            for (int i = 0; i < rectangle_vector.size(); i++)
            {
                window->draw(*rectangle_vector[i]);
            }
        }
        else
        {
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
                        window->draw(&vertex_vector[i], add, sf::Lines);
                        vertex_vector[i].position.x = prev_pos1;
                        vertex_vector[i + 1].position.x = prev_pos1;
                    }
                }
            }
            else window->draw(vertex_vector);
        }
        if (ACTIONS_DONE < changes.size())
        {
            for (int idx = ACTIONS_DONE; idx < std::min(int(changes.size()), ACTIONS_DONE + ACTIONS_TO_DO); idx++)
            {
                int i = changes[idx][0], j = changes[idx][1];
                int add = vertex_vector.getPrimitiveType() == sf::Lines ? 2 : 4;
                int i_idx = (i * add), j_idx = (j * add);
                if (changes[idx][2] != 3)
                {
                    if (!vertices_used) rectangle_vector[i]->setFillColor(NORMAL_COLOR);
                    else
                    {
                        for (int k = 0; k < add; k++)
                        {
                            vertex_vector[i_idx + k].color = NORMAL_COLOR;
                        }
                    }
                    if (changes[idx][2] != 2)
                    {
                        if (!vertices_used) rectangle_vector[j]->setFillColor(NORMAL_COLOR);
                        else
                        {
                            for (int k = 0; k < add; k++)
                            {
                                vertex_vector[j_idx + k].color = NORMAL_COLOR;
                            }
                        }
                    }
                }
            }
        }

        while (!sound_deque.empty() && sound_deque.front()->getStatus() == sf::Sound::Stopped)
        {
            delete sound_deque.front();
            sound_deque.pop_front();
        }

        if (ACTIONS_TO_DO > 0)
        {
            int ratio = int((avg / cnt / HEIGHT) * 999);
            sf::Sound* sound = new sf::Sound(trimmed_buffer);
            sound->setVolume(SOUND_COEFF / 10.0);
            sound->setPitch(beep_pitches[ratio]);
            sound->play();
            sound_deque.push_back(sound);
            ACTIONS_DONE += ACTIONS_TO_DO;
        }
    }

    void sort()
    {
        if (IS_ANIMATION)
        {
            if (main_vector.size() > sort_to_max_limited[int(sort_chosen)]) populate_vector(sort_to_max_limited[int(sort_chosen)]);
            else repopulate_vector();
        }
        else
        {
            if (main_vector.size() > sort_to_max_unlimited[int(sort_chosen)]) populate_vector(sort_to_max_unlimited[int(sort_chosen)]);
            else repopulate_vector();
        }
        sf::Clock clock;
        comparisons = 0;
        writes = 0;

        int start = clock.getElapsedTime().asMilliseconds();

        if (sort_chosen == Sort::bubble) bubble_sort(true);
        else if (sort_chosen == Sort::coctail_shaker) coctail_shaker_sort(true);
        else if (sort_chosen == Sort::insertion) insertion_sort(true);
        else if (sort_chosen == Sort::quick) quick_sort(true);
        else if (sort_chosen == Sort::merge) merge_sort(true);
        else if (sort_chosen == Sort::heap) heap_sort(true);
        else if (sort_chosen == Sort::counting) counting_sort(true);
        else if (sort_chosen == Sort::radix) radix_sort(true);

        int end = clock.getElapsedTime().asMilliseconds();
        time_text.setString("Time: " + std::to_string(end - start) + " ms.");

        if (IS_ANIMATION)
        {
            repopulate_vector();

            can_sort = false;
            can_populate = false;

            if (sort_chosen == Sort::bubble) bubble_sort();
            else if (sort_chosen == Sort::coctail_shaker) coctail_shaker_sort();
            else if (sort_chosen == Sort::insertion) insertion_sort();
            else if (sort_chosen == Sort::quick) quick_sort();
            else if (sort_chosen == Sort::merge) merge_sort();
            else if (sort_chosen == Sort::heap) heap_sort();
            else if (sort_chosen == Sort::counting) counting_sort();
            else if (sort_chosen == Sort::radix) radix_sort();
        }
        else
        {
            for (int i = 0; i < main_vector.size(); i++)
            {
                if (!vertices_used)
                {
                    rectangle_vector[i]->setSize(sf::Vector2f(element_width, double(main_vector[i]) / (MAX_ELEMENT * 1.1) * HEIGHT));
                    rectangle_vector[i]->setPosition(sf::Vector2f(i * element_width, HEIGHT - rectangle_vector[i]->getSize().y));
                }
                else
                {
                    int i_idx = (i * (vertex_vector.getPrimitiveType() == sf::Lines ? 2 : 4));
                    vertex_vector[i_idx].position.y = HEIGHT - (double(main_vector[i]) / (MAX_ELEMENT * 1.1) * HEIGHT);
                    if (vertex_vector.getPrimitiveType() == sf::Quads) vertex_vector[i_idx + 3].position.y = HEIGHT - (double(main_vector[i]) / (MAX_ELEMENT * 1.1) * HEIGHT);
                }
            }
            changes.push_back({ 0, 0, 0 });
        }
    }

    void bubble_sort(bool true_sort = false)
    {
        for (int i = 0; i < size; i++)
        {
            for (int j = 1; j < size - i; j++)
            {
                if (compare(j - 1, j, [](int a, int b) {return a > b; }, true_sort)) swap(j - 1, j, true_sort);
            }
        }
    }

    void coctail_shaker_sort(bool true_sort = false)
    {
        for (int i = 0; i < size / 2 + 1; i++)
        {
            bool sorted = true;
            for (int j = i + 1; j < size - i; j++)
            {
                if (compare(j - 1, j, [](int a, int b) {return a > b; }, true_sort))
                {
                    swap(j - 1, j, true_sort);
                    sorted = false;
                }
            }
            if (sorted) break;
            sorted = true;
            for (int j = size - 1 - i; j >= 1 + i; j--)
            {
                if (compare(j - 1, j, [](int a, int b) {return a > b; }, true_sort))
                {
                    swap(j - 1, j, true_sort);
                    sorted = false;
                }
            }
            if (sorted) break;
        }
    }

    void insertion_sort(bool true_sort = false)
    {
        for (int i = 1; i < size; i++)
        {
            for (int j = i; j > 0; j--)
            {
                if (compare(j - 1, j, [](int a, int b) {return a > b; }, true_sort)) swap(j - 1, j, true_sort);
                else break;
            }
        }
    }

    void quick_sort(bool true_sort = false)
    {
        quick_sort_helper(0, size - 1, true_sort);
    }

    void quick_sort_helper(int left, int right, bool true_sort)
    {
        if (left >= right) return;
        int mid = (left + right) / 2;
        if (mid != right) swap(mid, right, true_sort);
        int idx = left;
        for (int i = left; i < right; i++)
        {
            if (compare(i, right, [](int a, int b) {return a < b; }, true_sort))
            {
                swap(i, idx, true_sort);
                idx++;
            }
        }
        if (idx != right) swap(idx, right, true_sort);
        int right_idx = idx + 1;
        for (int i = idx + 1; i <= right; i++)
        {
            if (compare(i, idx, [](int a, int b) {return a == b; }, true_sort))
            {
                swap(i, right_idx, true_sort);
                right_idx++;
            }
        }
        quick_sort_helper(left, idx - 1, true_sort);
        quick_sort_helper(right_idx, right, true_sort);
    }

    void merge_sort(bool true_sort = false)
    {
        merge_sort_helper(0, size - 1, true_sort);
    }

    void merge_sort_helper(int left, int right, bool true_sort)
    {
        if (left >= right) return;
        int mid = (left + right) / 2;
        merge_sort_helper(left, mid, true_sort);
        merge_sort_helper(mid + 1, right, true_sort);
        std::vector<int> arr(right - left + 1);
        int idx = 0;
        int left_idx = left, right_idx = mid + 1;
        while (left_idx <= mid || right_idx <= right)
        {
            if (!true_sort)
            {
                if (left_idx > mid) arr[idx++] = right_idx++;
                else if (right_idx > right) arr[idx++] = left_idx++;
                else
                {
                    if (compare(left_idx, right_idx, [](int a, int b) {return a < b; }, true_sort)) arr[idx++] = left_idx++;
                    else arr[idx++] = right_idx++;
                }
            }
            else
            {
                if (left_idx > mid) arr[idx++] = main_vector[right_idx++];
                else if (right_idx > right) arr[idx++] = main_vector[left_idx++];
                else
                {
                    if (compare(left_idx, right_idx, [](int a, int b) {return a < b; }, true_sort)) arr[idx++] = main_vector[left_idx++];
                    else arr[idx++] = main_vector[right_idx++];
                }
            }
        }

        if (!true_sort)
        {
            for (int i = left; i <= right; i++)
            {
                swap(i, arr[i - left], true_sort);
                for (int j = i - left + 1; j <= right - left; j++)
                {
                    if (arr[j] == i)
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
            if (!IS_ANIMATION) writes += 2 * arr.size();
            for (int i = 0; i < arr.size(); i++) main_vector[idx++] = arr[i];
        }
    }

    struct HeapNode
    {
        int idx = 0;
        HeapNode* parent = nullptr, * left = nullptr, * right = nullptr;
    };
    int current_tree_idx = 0;
    void build_complete_tree(HeapNode* head, bool true_sort)
    {
        if (head->idx * 2 + 1 < main_vector.size())
        {
            if (head->left == nullptr) head->left = new HeapNode();
            head->left->parent = head;
            head->left->idx = head->idx * 2 + 1;
            compare(head->idx, head->left->idx, [](int a, int b) {return a < b; }, true_sort);
            build_complete_tree(head->left, true_sort);

        }
        if (head->idx * 2 + 2 < main_vector.size())
        {
            if (head->right == nullptr) head->right = new HeapNode();
            head->right->parent = head;
            head->right->idx = head->idx * 2 + 2;
            compare(head->idx, head->right->idx, [](int a, int b) {return a < b; }, true_sort);
            build_complete_tree(head->right, true_sort);
        }
    }
    void heapify(HeapNode* head)
    {
        if (head == nullptr) return;
        heapify(head->left);
        heapify(head->right);
        if (head->parent != nullptr && orig_main_vector[head->idx] > orig_main_vector[head->parent->idx])
        {
            int temp = head->idx;
            head->idx = head->parent->idx;
            head->parent->idx = temp;
        }
    }
    int find_a_leaf(HeapNode* head)
    {
        if (head == nullptr) return -1;
        if (head->right != nullptr) return find_a_leaf(head->right);
        else if (head->left != nullptr) return find_a_leaf(head->left);
        else
        {
            if (head->parent != nullptr)
            {
                if (head->parent->right != nullptr) head->parent->right = nullptr;
                else head->parent->left = nullptr;
            }
            int idx = head->idx;
            delete head;
            return idx;
        }
    }
    void fill_array(HeapNode* head, std::vector<std::vector<int>>& arr, int level = 1)
    {
        if (head == nullptr) return;
        if (arr.size() < level) arr.push_back({});
        arr[level - 1].push_back(head->idx);
        
        fill_array(head->left, arr, level + 1);
        fill_array(head->right, arr, level + 1);
    }
    void heap_sort(bool true_sort = false)
    {
        HeapNode* head = new HeapNode;

        int max_num = 0, max_num_idx = -1;
        for (int i = 0; i < main_vector.size(); i++)
        {
            if (main_vector[i] > max_num)
            {
                max_num = main_vector[i];
                max_num_idx = i;
            }
        }
        head->idx = max_num_idx;

        head->right = new HeapNode;
        head->right->idx = 0;
        head->right->parent = head;

        current_tree_idx = 0;
        build_complete_tree(head->right, true_sort);

        heapify(head->right);
        std::vector<std::vector<int>> arr;
        int idx;
        if (!true_sort)
        {
            fill_array(head->right, arr);
            idx = 0;
            for (int i = 0; i < arr.size(); i++)
            {
                for (int j = 0; j < arr[i].size(); j++)
                {
                    good_swap(idx, arr[i][j], true_sort);
                    idx++;
                }
            }
        }

        idx = main_vector.size() - 1;
        while (head->right != nullptr)
        {
            if (!true_sort)
            {
                swap(idx, new_idxs[head->right->idx], true_sort);
                for (int i = 0; i < main_vector.size(); i++)
                {
                    if (new_idxs[i] == idx)
                    {
                        new_idxs[i] = new_idxs[head->right->idx];
                        new_idxs[head->right->idx] = idx;
                        break;
                    }
                }
                compare(idx, new_idxs[head->right->idx], [](int a, int b) {return a < b; }, true_sort);
            }
            else
            {
                main_vector[idx] = orig_main_vector[head->right->idx];
                if (!IS_ANIMATION) writes++;
            }
            idx--;
            int new_idx = find_a_leaf(head->right);
            if (head->right == nullptr) break;
            head->right->idx = new_idx;
            heapify(head->right);
        }
    }

    void counting_sort(bool true_sort = false)
    {
        std::vector<std::pair<int, int>> arr(MAX_ELEMENT + 1, std::pair<int, int>(0, 0));
        writes += MAX_ELEMENT;
        for (int i = 0; i < size; i++)
        {
            arr[main_vector[i]].first++;
            arr[main_vector[i]].second = i;
            compare(i, i, [](int a, int b) {return a < b; }, true_sort);
        }
        int idx = 0;
        for (int i = 0; i < arr.size(); i++)
        {
            while (arr[i].first > 0)
            {
                arr[i].first--;
                set(idx++, arr[i].second, true_sort);
            }
        }
    }

    void radix_sort(bool true_sort = false)
    {
        int max_element = 0, max_element_idx = 0;
        for (int i = 0; i < main_vector.size(); i++)
        {
            if (compare(i, max_element_idx, [](int a, int b) {return a > b; }, true_sort))
            {
                max_element = main_vector[i];
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
                int current_digit = (main_vector[i] / divisor) % 10;
                counting_arr[current_digit].push_back(new_idxs[i]);
            }
            int idx = 0;
            for (int i = 0; i < 10; i++)
            {
                for (int j = 0; j < counting_arr[i].size(); j++)
                {
                    set(idx, counting_arr[i][j], true_sort);
                    new_idxs[idx] = counting_arr[i][j];
                    idx++;
                }
            }
            divisor *= 10;
        }
    }

    int MAX_ELEMENT = 1000;
    bool done_phase = false, can_sort = true, can_populate = true, sorted = false, stopped = false;
    std::vector<int> sort_to_max_limited = { 2000, 2000, 2000, 10000, 10000, 10000, 20000, 20000};
    std::vector<int> sort_to_max_unlimited = { 40000, 40000, 40000, 500000, 500000, 40000, 500000, 500000 };
    Sort sort_chosen = Sort::bubble;
private:
    bool vertices_used = false;
    sf::RenderWindow* window;

    std::vector<int> main_vector, orig_main_vector;
    std::vector<sf::RectangleShape*> rectangle_vector, orig_rectangle_vector;
    sf::VertexArray vertex_vector, orig_vertex_vector;
    std::vector<int> new_idxs;

    std::vector<std::vector<int>> changes;
    int ACTIONS_DONE = 0;
    double element_width;
    int size;
    int milliseconds_to_sort = -1;
    
    sf::SoundBuffer sound_buffer;
    sf::SoundBuffer trimmed_buffer;
    std::vector<float> beep_pitches;
    std::deque<sf::Sound*> sound_deque;

    sf::Text info_text, stat_text, time_text;
    sf::Font font;
    int comparisons = 0;
    int writes = 0;
    std::vector<std::string> sort_info = { "Bubble Sort",
                                            "Shaker Sort",
                                            "Insertion Sort",
                                            "Quick Sort",
                                            "Merge Sort",
                                            "Heap Sort",
                                            "Counting Sort", 
                                            "Radix Sort"};
};


class Button
{
public:
    Button() {}
    Button(float x_, float y_, float width_, float height_, std::string text_, sf::RenderWindow* window_, MainVector* main_vec_, bool holdable_ = true, Button* parent_ = nullptr, bool unpress_on_press_ = false, bool is_pressable_when_sorting_ = true, sf::Color NEW_NORMAL_COLOR = sf::Color::Transparent, sf::Color NEW_HOVERED_COLOR = sf::Color::Transparent, sf::Color NEW_PRESSED_COLOR = sf::Color::Transparent) : x(x_), y(y_), width(width_), height(height_), holdable(holdable_), text_string(text_)
    {
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
    }
    bool is_pressed() const
    {
        return pressed;
    }
    void press()
    {
        if (!is_pressable_when_sorting and !main_vec->can_sort) return;
        pressed = true;
        rect.setFillColor(PRESSED_BUTTON_COLOR);

        for (int i = 0; i < string_to_sort.size(); i++)
        {
            if (!holdable && string_to_sort[i].first == text_string)
            {
                main_vec->set_sort(string_to_sort[i].second);
                parent->set_string(text_string);
                break;
            }
        }
        if (text_string == "START")
        {
            if (main_vec->can_sort)
            {
                main_vec->sort();
            }
        }
        if (text_string == "STOP")
        {
            if (main_vec->is_stopped()) main_vec->unstop();
            else main_vec->stop();
        }
        if (!holdable) unpress();
    }

    void unpress()
    {
        pressed = false;
        rect.setFillColor(HOVERED_BUTTON_COLOR);
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
    void update(sf::Vector2i mouse_pos, bool mouse_pressed)
    {
        if (rect.getGlobalBounds().contains(sf::Vector2f(mouse_pos.x, mouse_pos.y)))
        {
            hovered = true;
            if (rect.getFillColor() == NORMAL_BUTTON_COLOR) rect.setFillColor(HOVERED_BUTTON_COLOR);
            if (mouse_pressed)
            {
                if (pressed) unpress();
                else press();
            }
        }
        else
        {
            if (mouse_pressed && unpress_on_press) unpress();
            hovered = false;
            if (rect.getFillColor() == HOVERED_BUTTON_COLOR) rect.setFillColor(NORMAL_BUTTON_COLOR);
        }
    }
    void draw()
    {
        window->draw(rect);
        window->draw(text);
    }
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

    bool hovered = false, pressed = false, holdable = true, unpress_on_press = false;
    bool is_pressable_when_sorting = true;

    sf::RenderWindow* window = nullptr;
    MainVector* main_vec = nullptr;
    Button* parent = nullptr;

    std::vector<std::pair<std::string, Sort>> string_to_sort = { {"BUBBLE", Sort::bubble},
                                                                {"SHAKER", Sort::coctail_shaker},
                                                                {"INSERTION", Sort::insertion},
                                                                {"QUICK", Sort::quick},
                                                                {"MERGE", Sort::merge},
                                                                {"HEAP", Sort::heap},
                                                                {"COUNTING", Sort::counting},
                                                                { "RADIX", Sort::radix }};
};

class DropDownButton
{
public:
    DropDownButton(float x_, float y_, float width_, float height_, std::string text_, std::vector<std::string> choices_, sf::RenderWindow* window_, MainVector* main_vec_)
    {
        head = new Button(x_, y_, width_, height_, text_, window_, main_vec_, true, nullptr, true, false);
        float current_x = x_, current_y = y_ + height_;
        choices.resize(choices_.size(), nullptr);
        for (int i = 0; i < choices.size(); i++)
        {
            choices[i] = new Button(current_x, current_y, width_, height_ * 0.8, choices_[i], window_, main_vec_, false, head);
            current_y += height_ * 0.8;
        }
    }
    void update(sf::Vector2i mouse_pos, bool mouse_pressed)
    {
        if (head->is_pressed())
        {
            for (int i = 0; i < choices.size(); i++)
                choices[i]->update(mouse_pos, mouse_pressed);
        }
        head->update(mouse_pos, mouse_pressed);
    }
    void draw()
    {
        head->draw();
        if (head->is_pressed())
        {
            for (int i = 0; i < choices.size(); i++)
                choices[i]->draw();
        }
    }
private:
    Button* head = nullptr;
    std::vector<Button*> choices;
};

class Slider
{
public:
    Slider(float x_, float y_, float width_, float height_, std::string text_, std::pair<int, int> range_, int* changing_value_, sf::RenderWindow* window_, MainVector* main_vec_, std::pair<int, int> max_range_ = {-1, -1}) : x(x_), y(y_), width(width_), height(height_), text_string(text_), range(range_), max_range(max_range_)
    {
        window = window_;
        main_vec = main_vec_;
        changing_value = changing_value_;

        slider_width = width * 0.8;

        line.setSize(sf::Vector2f(slider_width + 1, 1));
        line.setPosition(sf::Vector2f(x, y + (height / 2)));
        line.setFillColor(sf::Color(150, 150, 230));

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
    void update(sf::Vector2i mouse_pos, bool mouse_pressed)
    {
        if (!mouse_pressed) return;
        if (rect.getGlobalBounds().contains(sf::Vector2f(mouse_pos.x, mouse_pos.y)))
        {
            double ratio = (mouse_pos.x - x) / slider_width;
            moving_rect.setPosition(sf::Vector2f(x + slider_width * ratio, y + height / 2));
            int new_value = (ratio >= 0.9 && max_range.second != -1) ? (max_range.first + (((ratio - 0.9) / 0.1) * (max_range.second - max_range.first))) : range.first + int((range.second - range.first) * ratio);
            *changing_value = new_value;
            value_text.setString(std::to_string(*changing_value));
        }
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
        window->draw(moving_rect);
    }
private:
    float x, y, width, height;
    float slider_width;
    std::pair<int, int> range, max_range;
    int* changing_value;

    sf::Font font;
    sf::Text text, value_text;
    std::string text_string;

    sf::RectangleShape line, rect, moving_rect;
    sf::RenderWindow* window = nullptr;
    MainVector* main_vec = nullptr;
};


class TextWindow
{
public:
    TextWindow(float x_, float y_, float width_, float height_, std::string name_, std::vector<std::string> texts, std::vector<std::pair<int, int>> ranges_, std::vector<int> values_, sf::RenderWindow* window_, MainVector* main_vec_, int button_amount = 1) : x(x_), y(y_), width(width_), height(height_)
    {
        name = name_;
        font.loadFromFile("regular.ttf");

        window = window_;
        main_vec = main_vec_;
        ranges = ranges_;
        values = values_;
        set_button = new Button(x, y + height + 2, width, height / 2, "SET", window, main_vec, true);

        main_buttons.resize(button_amount, nullptr);
        for (int i = 0; i < button_amount; i++)
        {
            main_buttons[i] = new Button(x + (i * width / button_amount), y, width / button_amount, height, std::to_string(values[i]), window, main_vec, true, nullptr, true, true, sf::Color(220, 220, 220), sf::Color(190, 190, 190), sf::Color(160, 160, 160));
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

    void update(sf::Vector2i mouse_pos, bool mouse_pressed, sf::Keyboard::Scan::Scancode key_pressed = sf::Keyboard::Scan::Unknown)
    {
        for (int i = 0; i < main_buttons.size(); i++)
        {
            main_buttons[i]->update(mouse_pos, mouse_pressed);
        }

        set_button->update(mouse_pos, mouse_pressed);

        if (set_button->is_pressed())
        {
            if (name == "SETTINGS")
            {
                POPULATION = std::max(ranges[0].first, values[0]);
                main_vec->MAX_ELEMENT = std::max(ranges[1].first, values[1]);
                main_vec->populate_vector(POPULATION);
            }
            set_button->unpress();
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
                    if (values[0] > main_vec->sort_to_max_limited[int(main_vec->sort_chosen)]) values[0] = main_vec->sort_to_max_limited[int(main_vec->sort_chosen)];
                }
                else
                {
                    if (values[0] > main_vec->sort_to_max_unlimited[int(main_vec->sort_chosen)]) values[0] = main_vec->sort_to_max_unlimited[int(main_vec->sort_chosen)];
                }
                if (values[1] > ranges[1].second) values[1] = ranges[1].second;
            }
            main_buttons[i]->set_string(std::to_string(values[i]));
        }
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

class CheckBox
{
public:
    CheckBox(float x, float y, float width, std::string text_, int* changing_value_, std::pair<int, int> set_values_, bool default_state, sf::RenderWindow* window_, MainVector* main_vec_)
    {
        main_vec = main_vec_;
        set_values = set_values_;
        changing_value = changing_value_;
        window = window_;

        check_button = new Button(x, y, width, width, "", window_, main_vec_, true, nullptr, false, true, sf::Color(240, 240, 240), sf::Color(200, 255, 200), sf::Color(30, 255, 30));
        if (default_state) check_button->press();

        font.loadFromFile("regular.ttf");
        text.setFont(font);
        text.setString(text_);
        text.setCharacterSize(16);
        text.setFillColor(sf::Color(30, 30, 30));
        sf::FloatRect text_rect = text.getLocalBounds();
        text.setOrigin(text_rect.getSize().x, text_rect.getSize().y / 2);
        text.setPosition(sf::Vector2f(x - 40, y + width / 2 - 5));
    }
    void draw()
    {
        window->draw(text);
        check_button->draw();
    }
    void update(sf::Vector2i mouse_pos, bool mouse_pressed)
    {
        if (!main_vec->can_sort) return;
        check_button->update(mouse_pos, mouse_pressed);
        if (check_button->is_pressed()) *changing_value = set_values.second;
        else *changing_value = set_values.first;
    }
private:
    sf::RenderWindow* window;
    MainVector* main_vec;
    sf::Font font;
    sf::Text text;
    Button* check_button;
    int* changing_value;
    std::pair<int, int> set_values;
};

int main()
{
    srand(time(nullptr));
    sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "Sorting", sf::Style::Titlebar | sf::Style::Close);
    sf::Image icon;
    icon.loadFromFile("icon.png");
    window.setFramerateLimit(60);
    window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());

    sf::RectangleShape menu_rect(sf::Vector2f(WIDTH - SORT_WIDTH, HEIGHT));
    menu_rect.setPosition(sf::Vector2f(SORT_WIDTH + 4, 0));
    menu_rect.setFillColor(sf::Color(220, 220, 220));
    menu_rect.setOutlineThickness(4);
    menu_rect.setOutlineColor(sf::Color(100, 100, 100));

    POPULATION = 100;
    MainVector vec(POPULATION, &window);
    vec.set_sort(Sort::bubble);

    Button start_button(SORT_WIDTH + 12, HEIGHT - 70, (WIDTH - SORT_WIDTH) - 20, 60, "START", &window, &vec, false);
    Button stop_button(SORT_WIDTH + 12, HEIGHT - 110, (WIDTH - SORT_WIDTH) - 20, 30, "STOP", &window, &vec, false);
    DropDownButton sort_choice_button(SORT_WIDTH + 12, 10, (WIDTH - SORT_WIDTH) - 20, 50, "SORT CHOICE", { "BUBBLE", "SHAKER", "INSERTION", "QUICK", "MERGE", "HEAP", "COUNTING", "RADIX"}, &window, &vec);
    Slider animation_speed_slider(SORT_WIDTH + 12, HEIGHT - 260, (WIDTH - SORT_WIDTH) - 20, 40, "ANIM. SPEED", { 1, 100 }, &ACTIONS_PER_FRAME, &window, &vec, {100, 1000});
    Slider sound_slider(SORT_WIDTH + 12, HEIGHT - 320, (WIDTH - SORT_WIDTH) - 20, 40, "SOUND", { 0, 100 }, &SOUND_COEFF, &window, &vec);
    CheckBox animate_checkbox(WIDTH - 30, HEIGHT - 370, 20, "ANIMATE?", &IS_ANIMATION, { 0, 1 }, true, &window, &vec);
    animation_speed_slider.set(ACTIONS_PER_FRAME);
    sound_slider.set(SOUND_COEFF);

    TextWindow population_window(SORT_WIDTH + 12, HEIGHT - 190, (WIDTH - SORT_WIDTH) - 20, 40, "SETTINGS", { "SIZE", "MAX" }, { { 5, 2000 }, {2, 100000000} }, {POPULATION, vec.MAX_ELEMENT}, &window, &vec, 2);

    while (window.isOpen())
    {
        sf::Event event;
        bool mouse_pressed_this_frame = false, mouse_pressed = false;
        sf::Keyboard::Scan::Scancode key_pressed = sf::Keyboard::Scan::Unknown;
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

        start_button.update(mouse_pos, mouse_pressed_this_frame);
        stop_button.update(mouse_pos, mouse_pressed_this_frame);

        sort_choice_button.update(mouse_pos, mouse_pressed_this_frame);

        animation_speed_slider.update(mouse_pos, mouse_pressed);
        sound_slider.update(mouse_pos, mouse_pressed);

        animate_checkbox.update(mouse_pos, mouse_pressed_this_frame);
        population_window.update(mouse_pos, mouse_pressed_this_frame, key_pressed);

        window.clear(BACKGROUND_COLOR);
        window.draw(menu_rect);

        vec.draw();
        start_button.draw();
        stop_button.draw();
        sort_choice_button.draw();
        animation_speed_slider.draw();
        population_window.draw();
        sound_slider.draw();
        animate_checkbox.draw();

        window.display();
    }
}
