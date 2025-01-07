#include <iostream>
#include <utility>
#include <iterator>
#include <cstddef>

#include <map>

template <typename Key, typename Value, typename Compare = std::less<Key>>
class Map {
    using value_type = std::pair<const Key, Value>;

    template<class Iter, class NodeType>
    struct Insert_return_type
    {
            Iter     position;
            bool     inserted;
            NodeType node;
    };

    struct Base_node {
        Base_node* parent   = nullptr;
        Base_node* left     = nullptr;
        Base_node* right    = nullptr;
        bool red  = false;

        Base_node() = default;

        Base_node(const Base_node&) = delete;
        Base_node(Base_node&) = delete;

        Base_node& operator=(Base_node&) = delete;
        Base_node& operator=(const Base_node&) = delete;

        virtual ~Base_node() = default;
    };

    struct Node final : public Base_node {
        value_type kv;

        explicit Node(const value_type& data) : kv(data) {}

        Node() = delete;

        Node(const Node&) = delete;
        Node(Node&) = delete;

        Node& operator=(Node&) = delete;
        Node& operator=(const Node&) = delete;

        ~Node()override = default;
    };

    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using key_compare = Compare;

    using reference = value_type&;
    using const_reference = const value_type&;

    using pointer = value_type*;
    using const_pointer = const value_type*;

    using mapped_type = Value;
    using node_type = Node;

    Base_node* root_ = nullptr;

    template<const bool is_const>
    class Base_iterator {
    public:
        Base_node* current = nullptr;

        using iterator_category =   std::bidirectional_iterator_tag;
        using value_type        =   std::conditional_t<is_const, std::pair<const Key, const Value>, std::pair<const Key, Value>>;
        using difference_type   =   std::ptrdiff_t;
        using pointer           =   value_type*;
        using reference         =   value_type&;

        Base_iterator() = default;
        Base_iterator(Base_node* node) : current(node) {}
        Base_iterator(const Base_iterator& base) {
            current = base.current;
        }

        [[nodiscard]] Base_node* get_base_node() {
            return current;
        }

        reference operator*() const {
            return static_cast<Node*>(current)->kv;
        }

        pointer operator->() const {
            return &(static_cast<Node*>(current)->kv);
        }

        bool operator==(const Base_iterator& other) const {
            return current == other.current;
        }

        bool operator!=(const Base_iterator& other) const {
            return current != other.current;
        }

        bool operator<(const Base_iterator& other) const {
            return current < other.current;
        }

        bool operator>(const Base_iterator& other) const {
            return current > other.current;
        }

        bool operator<=(const Base_iterator& other) const {
            return current <= other.current;
        }

        bool operator>=(const Base_iterator& other) const {
            return current >= other.current;
        }

        auto operator<=>(const Base_iterator& other) const = default;

        operator Base_iterator<true>() const {
            return Base_iterator<true>(current);
        }
    };

public:
    Map() = default;

    using iterator = Base_iterator<false>;
    using const_iterator = Base_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    iterator begin() noexcept {
        return iterator(root_->left);
    }

    const_iterator begin() const noexcept {
        return const_iterator(root_->left);
    }

    const_iterator cbegin() const noexcept {
        return begin();
    }

    iterator end() noexcept {
        return iterator(root_->right);
    }

    const_iterator end() const noexcept {
        return const_iterator(root_->right);
    }

    const_iterator cend() const noexcept {
        return end();
    }

    reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }

    const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    const_reverse_iterator crbegin() const noexcept {
        return rbegin();
    }

    reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    const_reverse_iterator crend() const noexcept {
        return rend();
    }

    std::pair<iterator, bool> insert(const value_type& value) {
        if(root_ == nullptr) {
            root_ = new Base_node;
            root_->parent = new Node(value);

            return std::make_pair(iterator(root_->parent), true);
        }

        Node* node = static_cast<Node*>(root_->parent);

        while(node != nullptr) {
            if(node->kv.first < value.first) {
                if(node->left != nullptr) {
                    node = static_cast<Node*>(node->left);
                } else {
                    root_->left = node->left = new Node(value);
                    return {iterator(node->left), true};
                }
            } else if(node->kv.first > value.first) {
                if(node->right != nullptr) {
                    node = static_cast<Node*>(node->right);
                } else {
                    root_->right = node->right = new Node(value);
                    return {iterator(node->right), true};
                }
            } else {
                return {iterator(node), false};
            }
        }

        return {iterator(nullptr), false};
    }
    [[nodiscard]] iterator find( const Key& key ) const {
        if(root_ == nullptr) {
            return iterator(nullptr);
        }

        Node* node = static_cast<Node*>(root_->parent);

        while(node != nullptr) {
            if(node->kv.first < key) {
                node = static_cast<Node*>(node->left);
            }
            else if(node->kv.first > key) {
                node = static_cast<Node*>(node->right);
            } else {
                return iterator(node);
            }
        }

        return iterator(nullptr);
    }

    Value& operator[](const Key& key) {
        std::pair<iterator, bool> it = insert({key, Value()});

        return it.first->second;
    }

    [[nodiscard]] size_type count( const Key& key ) const {
        if(find(key).current == nullptr) {
            return 0;
        }

        return 1;
    }

    [[nodiscard]] const Value& at( const Key& key ) const {
        if (count(key)) {
            return find(key)->second; // Возвращаем значение по найденному ключу
        }

        throw std::out_of_range("Key not found"); // Исключение, если ключ не найден
    }

    iterator erase( iterator pos ) {
        if(root_ == nullptr) {
            return iterator(nullptr);
        }

        //pos.get_base_node()->parent->left = pos.get_base_node()->left;
        //pos.get_base_node()->parent->right = pos.get_base_node()->right;

        //pos.current->parent->left = pos.current->

        if(pos->first < static_cast<Node*>(root_->parent)->kv.first) {
            pos.current->parent->left = pos.current->right;
            pos.current->right->parent = pos.current->parent;
            pos.current->right->left = pos.current->left;
        } else {

        }

        return iterator(nullptr);
    }

    iterator erase( const_iterator pos ) {

    }

    iterator erase( iterator first, iterator last ) {

    }

    iterator erase( const_iterator first, const_iterator last ) {

    }

    size_type erase( const Key& key ) {

    }

};

int main() {
    Map<std::string, int> map;
    map.insert({"Apple", 200});

    map.erase(map.find("Apple"));

    std::cout << map.count("Apple6") << std::endl;
    std::cout << map.find("Apple")->second << std::endl;
}