#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <string>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>
#include <cctype>

float clampFloat(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

struct Button {
    sf::RectangleShape rect;
    std::string text;
    unsigned int charSize;
    bool pressed;
    sf::Color baseColor;
    sf::Color pressedColor;

    Button(
        const std::string& t,
        const sf::Font& ,
        unsigned int cs,
        sf::Vector2f pos,
        sf::Vector2f size,
        sf::Color color
    )
        : text(t)
        , charSize(cs)
        , pressed(false)
        , baseColor(color)
        , pressedColor(color)
    {
        rect.setPosition(pos);
        rect.setSize(size);
        rect.setFillColor(baseColor);
        rect.setOutlineColor(sf::Color(50, 50, 50));
        rect.setOutlineThickness(2.f);
    }

    bool contains(sf::Vector2f p) const {
        sf::Vector2f pos = rect.getPosition();
        sf::Vector2f s = rect.getSize();
        return (p.x >= pos.x && p.x <= pos.x + s.x &&
            p.y >= pos.y && p.y <= pos.y + s.y);
    }

    void setPressed(bool v) {
        pressed = v;
        rect.setFillColor(pressed ? pressedColor : baseColor);
    }

    void draw(sf::RenderWindow& win, const sf::Font& font) const {
        win.draw(rect);

        sf::Text label(font);
        label.setString(text);
        label.setCharacterSize(charSize);
        label.setFillColor(sf::Color::White);

        sf::FloatRect lb = label.getLocalBounds();
        sf::Vector2f origin(
            lb.position.x + lb.size.x / 2.f,
            lb.position.y + lb.size.y / 2.f
        );
        label.setOrigin(origin);
        label.setPosition(rect.getPosition() + rect.getSize() / 2.f);

        win.draw(label);
    }
};

struct Tile {
    char letter;
    int  score;

    sf::RectangleShape rect;
    sf::Text letterText;
    sf::Text scoreText;

    bool grabbed;
    sf::Vector2f grabOffset;

    int occupantSpace;   
    sf::Vector2f revertPosition;

    Tile(char c, int s, const sf::Font& font, float size = 64.f)
        : letter(c)
        , score(s)
        , rect({ size, size })
        , letterText(font, std::string(1, c), static_cast<unsigned int>(size * 0.6f))
        , scoreText(font, std::to_string(s), static_cast<unsigned int>(size * 0.24f))
        , grabbed(false)
        , grabOffset(0.f, 0.f)
        , occupantSpace(-1)
        , revertPosition(0.f, 0.f)
    {
        rect.setFillColor(sf::Color(245, 240, 210));
        rect.setOutlineColor(sf::Color(80, 80, 80));
        rect.setOutlineThickness(2.f);

        letterText.setStyle(sf::Text::Bold);
        letterText.setFillColor(sf::Color::Black);

        scoreText.setFillColor(sf::Color::Black);
    }

    void setPosition(const sf::Vector2f& pos) {
        rect.setPosition(pos);

        sf::Vector2f sz = rect.getSize();

        sf::FloatRect lb = letterText.getLocalBounds();
        sf::Vector2f originLetter(
            lb.position.x + lb.size.x / 2.f,
            lb.position.y + lb.size.y / 2.f
        );
        letterText.setOrigin(originLetter);
        letterText.setPosition(pos + sf::Vector2f(sz.x / 2.f, sz.y / 2.3f));

        sf::FloatRect sb = scoreText.getLocalBounds();
        sf::Vector2f originScore(
            sb.position.x + sb.size.x,
            sb.position.y + sb.size.y
        );
        scoreText.setOrigin(originScore);
        scoreText.setPosition(pos + sf::Vector2f(sz.x - 6.f, sz.y - 6.f));
    }

    sf::Vector2f getPosition() const {
        return rect.getPosition();
    }

    sf::Vector2f getSize() const {
        return rect.getSize();
    }

    sf::Vector2f getCenter() const {
        return getPosition() + getSize() / 2.f;
    }

    bool contains(sf::Vector2f p) const {
        sf::Vector2f pos = rect.getPosition();
        sf::Vector2f s = rect.getSize();
        return (p.x >= pos.x && p.x <= pos.x + s.x &&
            p.y >= pos.y && p.y <= pos.y + s.y);
    }

    void draw(sf::RenderWindow& win) const {
        win.draw(rect);
        win.draw(letterText);
        win.draw(scoreText);
    }
};

enum class Mult {
    NONE = 0,
    DOUBLE_LETTER,
    TRIPLE_LETTER,
    DOUBLE_WORD,
    TRIPLE_WORD
};

struct Space {
    sf::RectangleShape rect;
    int occupantPlayer;  
    int occupantIndex;  
    Mult mult;
    bool highlighted;

    Space()
        : occupantPlayer(-1)
        , occupantIndex(-1)
        , mult(Mult::NONE)
        , highlighted(false)
    {
    }

    Space(const sf::Vector2f& pos, float size)
        : occupantPlayer(-1)
        , occupantIndex(-1)
        , mult(Mult::NONE)
        , highlighted(false)
    {
        rect.setPosition(pos);
        rect.setSize(sf::Vector2f(size, size));
        rect.setFillColor(sf::Color(220, 230, 250));
        rect.setOutlineColor(sf::Color(50, 50, 100));
        rect.setOutlineThickness(2.f);
    }

    bool contains(sf::Vector2f p) const {
        sf::Vector2f pos = rect.getPosition();
        sf::Vector2f s = rect.getSize();
        return (p.x >= pos.x && p.x <= pos.x + s.x &&
            p.y >= pos.y && p.y <= pos.y + s.y);
    }

    sf::Vector2f getCenter() const {
        sf::Vector2f pos = rect.getPosition();
        sf::Vector2f s = rect.getSize();
        return pos + s / 2.f;
    }

    void applyMultiplierColorOrDefault() {
        switch (mult) {
        case Mult::DOUBLE_LETTER:
            rect.setFillColor(sf::Color(173, 216, 230));
            break;
        case Mult::TRIPLE_LETTER:
            rect.setFillColor(sf::Color(30, 90, 160));   
            break;
        case Mult::DOUBLE_WORD:
            rect.setFillColor(sf::Color(255, 165, 0));   
            break;
        case Mult::TRIPLE_WORD:
            rect.setFillColor(sf::Color(200, 40, 40));  
            break;
        default:
            rect.setFillColor(sf::Color(220, 230, 250));
            break;
        }
    }

    void setHighlight(bool on) {
        highlighted = on;
        if (on) {
            rect.setFillColor(sf::Color(200, 230, 255));
        }
        else {
            applyMultiplierColorOrDefault();
        }
    }

    void draw(sf::RenderWindow& win) const {
        win.draw(rect);
    }
};

void pushMany(std::vector<char>& bag, char c, int count) {
    for (int i = 0; i < count; ++i) {
        bag.push_back(c);
    }
}

void reflowRack(
    std::vector<Tile>& tiles,
    float regionStartX,
    float tileSize,
    float rackSpacing,
    float targetY
) {
    const float boardSpacing = 12.f;
    float regionWidth = 7.f * (tileSize + boardSpacing) - boardSpacing;

    float totalRackWidth = 0.f;
    if (!tiles.empty()) {
        totalRackWidth = static_cast<float>(tiles.size()) * tileSize +
            static_cast<float>(tiles.size() - 1) * rackSpacing;
    }

    float rackStartX = regionStartX + (regionWidth - totalRackWidth) / 2.f;

    for (std::size_t i = 0; i < tiles.size(); ++i) {
        sf::Vector2f pos(
            rackStartX + static_cast<float>(i) * (tileSize + rackSpacing),
            targetY
        );
        tiles[i].setPosition(pos);
        tiles[i].revertPosition = pos;
    }
}

bool isValidWord(const std::string& word)
{
    static std::unordered_set<std::string> words;
    static bool loaded = false;

    if (!loaded)
    {
        std::ifstream file("C:/Users/saadc/words_alpha.txt"); 
        if (!file.is_open())
        {
            std::cerr << "[WARN] Could not open dictionary file. "
                "Treating all words as valid.\n";
            loaded = true;
            return true;
        }

        std::string w;
        while (file >> w)
        {
            std::transform(
                w.begin(), w.end(), w.begin(),
                [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); }
            );
            words.insert(w);
        }
        loaded = true;
    }

    std::string check = word;
    std::transform(
        check.begin(), check.end(), check.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); }
    );

    if (words.empty())
    {
        return true;
    }

    if (words.find(check) != words.end())
    {
        std::cout << "Valid English word: " << check << "\n";
        return true;
    }
    else
    {
        std::cout << "Not found in dictionary: " << check << "\n";
        return false;
    }
}

int main() {
    const unsigned int WINDOW_W = 1100;
    const unsigned int WINDOW_H = 640;

    sf::RenderWindow window(
        sf::VideoMode({ WINDOW_W, WINDOW_H }),
        "Scrabble - 2 players with bag refill (SFML 3 + Dictionary)"
    );
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.openFromFile("C:/dejavu-sans-bold-webfont.ttf")) {
        std::cerr << "Failed to load font\n";
        return 1;
    }

    const int   NUM_SPACES = 7;
    const float spaceSize = 72.f;
    const float spacing = 12.f;

    const float totalWidth = NUM_SPACES * spaceSize + (NUM_SPACES - 1) * spacing;
    const float startX = (static_cast<float>(WINDOW_W) - 300.f - totalWidth) / 2.f;
    const float startY = 220.f;

    std::vector<Space> spaces;
    spaces.reserve(NUM_SPACES);
    for (int i = 0; i < NUM_SPACES; ++i) {
        sf::Vector2f pos(
            startX + static_cast<float>(i) * (spaceSize + spacing),
            startY
        );
        spaces.emplace_back(pos, spaceSize);
    }

    std::unordered_map<char, int> scoreMap;
    scoreMap['A'] = 1;  scoreMap['B'] = 3;  scoreMap['C'] = 3;  scoreMap['D'] = 2;
    scoreMap['E'] = 1;  scoreMap['F'] = 4;  scoreMap['G'] = 2;  scoreMap['H'] = 4;
    scoreMap['I'] = 1;  scoreMap['J'] = 8;  scoreMap['K'] = 5;  scoreMap['L'] = 1;
    scoreMap['M'] = 3;  scoreMap['N'] = 1;  scoreMap['O'] = 1;  scoreMap['P'] = 3;
    scoreMap['Q'] = 10; scoreMap['R'] = 1;  scoreMap['S'] = 1;  scoreMap['T'] = 1;
    scoreMap['U'] = 1;  scoreMap['V'] = 4;  scoreMap['W'] = 4;  scoreMap['X'] = 8;
    scoreMap['Y'] = 4;  scoreMap['Z'] = 10;

    std::vector<char> bag;
    pushMany(bag, 'E', 12);
    pushMany(bag, 'A', 9);
    pushMany(bag, 'I', 9);
    pushMany(bag, 'O', 8);
    pushMany(bag, 'N', 6);
    pushMany(bag, 'R', 6);
    pushMany(bag, 'T', 6);
    pushMany(bag, 'L', 4);
    pushMany(bag, 'S', 4);
    pushMany(bag, 'U', 4);
    pushMany(bag, 'D', 4);
    pushMany(bag, 'G', 3);
    pushMany(bag, 'B', 2);
    pushMany(bag, 'C', 2);
    pushMany(bag, 'M', 2);
    pushMany(bag, 'P', 2);
    pushMany(bag, 'F', 2);
    pushMany(bag, 'H', 2);
    pushMany(bag, 'V', 2);
    pushMany(bag, 'W', 2);
    pushMany(bag, 'Y', 2);
    pushMany(bag, 'K', 1);
    pushMany(bag, 'J', 1);
    pushMany(bag, 'X', 1);
    pushMany(bag, 'Q', 1);
    pushMany(bag, 'Z', 1);

    std::random_device rd;
    std::mt19937 rng(rd());
    std::shuffle(bag.begin(), bag.end(), rng);

    std::vector<Tile> racks[2];
    const float tileSize = 64.f;
    const float rackSpacing = 12.f;

    std::string p0 = "EXAMPLES";
    std::string p1 = "PLAYERS";

    for (char c : p0) {
        char up = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        int sc = scoreMap[up];
        racks[0].emplace_back(up, sc, font, tileSize);
    }
    for (char c : p1) {
        char up = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        int sc = scoreMap[up];
        racks[1].emplace_back(up, sc, font, tileSize);
    }

    auto drawOneFromBag = [&](int playerIdx) {
        if (bag.empty()) return;
        char c = bag.back();
        bag.pop_back();
        char up = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        int sc = 1;
        auto it = scoreMap.find(up);
        if (it != scoreMap.end()) sc = it->second;
        racks[playerIdx].emplace_back(up, sc, font, tileSize);
        };

    const float rackY_player0 = 340.f;
    const float rackY_player1 = 430.f;

    for (int pl = 0; pl < 2; ++pl) {
        while (static_cast<int>(racks[pl].size()) < 7 && !bag.empty()) {
            drawOneFromBag(pl);
        }
    }

    reflowRack(racks[0], startX, tileSize, rackSpacing, rackY_player0);
    reflowRack(racks[1], startX, tileSize, rackSpacing, rackY_player1);

    const float barHeight = 64.f;

    sf::RectangleShape barBg;
    barBg.setSize(sf::Vector2f(static_cast<float>(WINDOW_W) - 40.f, barHeight));
    barBg.setPosition(sf::Vector2f(20.f, 80.f));       
    barBg.setFillColor(sf::Color(30, 30, 60));        
    barBg.setOutlineColor(sf::Color(240, 240, 240));     
    barBg.setOutlineThickness(1.f);

    sf::RectangleShape barFill;
    barFill.setPosition(barBg.getPosition());
    barFill.setSize(sf::Vector2f(0.f, barHeight));
    barFill.setFillColor(sf::Color(80, 160, 240));       

    sf::Text scoreLabel(font);
    scoreLabel.setCharacterSize(20);
    scoreLabel.setStyle(sf::Text::Bold);
    scoreLabel.setFillColor(sf::Color::White);

    sf::Text totalLabelP0(font);
    totalLabelP0.setCharacterSize(16);
    totalLabelP0.setFillColor(sf::Color::White);

    sf::Text totalLabelP1(font);
    totalLabelP1.setCharacterSize(16);
    totalLabelP1.setFillColor(sf::Color::White);

    sf::Text turnLabel(font);
    turnLabel.setCharacterSize(18);
    turnLabel.setStyle(sf::Text::Bold);
    turnLabel.setFillColor(sf::Color::Yellow);

    sf::Text movesLabel(font);
    movesLabel.setCharacterSize(16);
    movesLabel.setFillColor(sf::Color::White);

    sf::Text bagCountText(font);
    bagCountText.setCharacterSize(16);
    bagCountText.setFillColor(sf::Color::White);

    sf::Text caption(font);
    caption.setString("Placed tiles score");
    caption.setCharacterSize(14);
    caption.setFillColor(sf::Color(200, 200, 200));
    caption.setPosition(barBg.getPosition() + sf::Vector2f(12.f, 40.f));

    // ---- Buttons ----
    std::vector<Button> buttons;
    const float btnW = 180.f;
    const float btnH = 50.f;
    const float btnX = startX + totalWidth + 40.f;
    float       btnY = rackY_player1 - 10.f - btnH * 5.f;
    const float btnGap = 12.f;

    sf::Color lightBlue(173, 216, 230);
    sf::Color darkBlue(30, 90, 160);
    sf::Color orange(255, 165, 0);
    sf::Color red(200, 40, 40);
    sf::Color commitColor(80, 160, 80);

    buttons.emplace_back("Double Letter", font, 16,
        sf::Vector2f(btnX, btnY + 0.f * (btnH + btnGap)),
        sf::Vector2f(btnW, btnH), lightBlue);

    buttons.emplace_back("Triple Letter", font, 16,
        sf::Vector2f(btnX, btnY + 1.f * (btnH + btnGap)),
        sf::Vector2f(btnW, btnH), darkBlue);

    buttons.emplace_back("Double Word", font, 16,
        sf::Vector2f(btnX, btnY + 2.f * (btnH + btnGap)),
        sf::Vector2f(btnW, btnH), orange);

    buttons.emplace_back("Triple Word", font, 16,
        sf::Vector2f(btnX, btnY + 3.f * (btnH + btnGap)),
        sf::Vector2f(btnW, btnH), red);

    Button commitBtn("Commit Move", font, 16,
        sf::Vector2f(btnX, btnY + 4.f * (btnH + btnGap)),
        sf::Vector2f(btnW, btnH), commitColor);

    int selectedButton = -1;
    int grabbedPlayer = -1;
    int grabbedIndex = -1;
    int prevOccupiedSpace = -1;

    int currentPlayer = 0;
    int movesDone = 0;
    const int MAX_MOVES = 20;
    int totals[2] = { 0, 0 };

    auto isGameOver = [&]() -> bool {
        return movesDone >= MAX_MOVES;
        };

    auto computePlacedScoreForPlayer = [&](int playerIdx) -> int {
        int letterSum = 0;
        int wordMult = 1;

        for (int i = 0; i < NUM_SPACES; ++i) {
            if (spaces[i].occupantPlayer == playerIdx &&
                spaces[i].occupantIndex >= 0)
            {
                int idx = spaces[i].occupantIndex;
                if (idx >= 0 && idx < static_cast<int>(racks[playerIdx].size())) {
                    int base = racks[playerIdx][idx].score;
                    switch (spaces[i].mult) {
                    case Mult::DOUBLE_LETTER: base *= 2; break;
                    case Mult::TRIPLE_LETTER: base *= 3; break;
                    case Mult::DOUBLE_WORD:   wordMult *= 2; break;
                    case Mult::TRIPLE_WORD:   wordMult *= 3; break;
                    default: break;
                    }
                    letterSum += base;
                }
            }
        }
        return letterSum * wordMult;
        };

    auto commitMove = [&]() {
        if (isGameOver()) return;
        std::string formedWord;
        for (int i = 0; i < NUM_SPACES; ++i) {
            if (spaces[i].occupantPlayer == currentPlayer &&
                spaces[i].occupantIndex >= 0)
            {
                int idx = spaces[i].occupantIndex;
                if (idx >= 0 && idx < static_cast<int>(racks[currentPlayer].size())) {
                    char ch = racks[currentPlayer][idx].letter;
                    char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                    formedWord += lower;
                }
            }
        }

        if (!formedWord.empty() && !isValidWord(formedWord)) {
            std::cout << "Move cancelled: invalid word: " << formedWord << "\n";
            return;
        }

        int moveScore = computePlacedScoreForPlayer(currentPlayer);
        totals[currentPlayer] += moveScore;

        std::vector<int> toRemove;
        for (int i = 0; i < NUM_SPACES; ++i) {
            if (spaces[i].occupantPlayer == currentPlayer &&
                spaces[i].occupantIndex >= 0)
            {
                toRemove.push_back(spaces[i].occupantIndex);
            }
        }
        if (!toRemove.empty()) {
            std::sort(toRemove.begin(), toRemove.end(), std::greater<int>());
            for (int idx : toRemove) {
                if (idx >= 0 && idx < static_cast<int>(racks[currentPlayer].size())) {
                    racks[currentPlayer].erase(racks[currentPlayer].begin() + idx);
                }
            }
        }

        for (Space& sp : spaces) {
            sp.occupantPlayer = -1;
            sp.occupantIndex = -1;
            sp.mult = Mult::NONE;
            sp.applyMultiplierColorOrDefault();
        }

        while (static_cast<int>(racks[currentPlayer].size()) < 7 && !bag.empty()) {
            drawOneFromBag(currentPlayer);
        }
        reflowRack(racks[0], startX, tileSize, rackSpacing, rackY_player0);
        reflowRack(racks[1], startX, tileSize, rackSpacing, rackY_player1);

        selectedButton = -1;
        for (Button& b : buttons) b.setPressed(false);

        movesDone++;
        currentPlayer = 1 - currentPlayer;

        grabbedIndex = -1;
        grabbedPlayer = -1;
        prevOccupiedSpace = -1;
        };

    sf::Text help(font);
    help.setCharacterSize(14);
    help.setFillColor(sf::Color::White);

    while (window.isOpen()) {
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
                continue;
            }

            if (isGameOver())
                continue;

            if (const auto* mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                sf::Vector2i mpix = mouseButtonPressed->position;
                sf::Vector2f mp = window.mapPixelToCoords(mpix);

                if (mouseButtonPressed->button == sf::Mouse::Button::Right) {
                    for (int i = 0; i < NUM_SPACES; ++i) {
                        if (spaces[i].contains(mp)) {
                            if (spaces[i].mult != Mult::NONE) {
                                spaces[i].mult = Mult::NONE;
                                spaces[i].applyMultiplierColorOrDefault();
                            }
                            break;
                        }
                    }
                    continue;
                }
                if (mouseButtonPressed->button == sf::Mouse::Button::Left) {
                    bool btnHandled = false;
                    for (std::size_t i = 0; i < buttons.size(); ++i) {
                        if (buttons[i].contains(mp)) {
                            selectedButton = static_cast<int>(i);
                            for (std::size_t j = 0; j < buttons.size(); ++j)
                                buttons[j].setPressed(static_cast<int>(j) == selectedButton);
                            btnHandled = true;
                            break;
                        }
                    }
                    if (btnHandled)
                        continue;
                    if (commitBtn.contains(mp)) {
                        bool hasPlaced = false;
                        for (int i = 0; i < NUM_SPACES; ++i) {
                            if (spaces[i].occupantPlayer == currentPlayer) {
                                hasPlaced = true;
                                break;
                            }
                        }
                        if (hasPlaced)
                            commitMove();
                        continue;
                    }
                    grabbedPlayer = currentPlayer;
                    grabbedIndex = -1;
                    for (int i = static_cast<int>(racks[grabbedPlayer].size()) - 1; i >= 0; --i) {
                        if (racks[grabbedPlayer][i].contains(mp)) {
                            grabbedIndex = i;
                            Tile& t = racks[grabbedPlayer][i];
                            t.grabbed = true;
                            t.grabOffset = mp - t.getPosition();
                            t.revertPosition = t.getPosition();

                            prevOccupiedSpace = t.occupantSpace;
                            if (prevOccupiedSpace != -1 &&
                                prevOccupiedSpace >= 0 &&
                                prevOccupiedSpace < static_cast<int>(spaces.size()))
                            {
                                if (spaces[prevOccupiedSpace].occupantPlayer == grabbedPlayer &&
                                    spaces[prevOccupiedSpace].occupantIndex == grabbedIndex)
                                {
                                    spaces[prevOccupiedSpace].occupantPlayer = -1;
                                    spaces[prevOccupiedSpace].occupantIndex = -1;
                                }
                                t.occupantSpace = -1;
                            }
                            else {
                                prevOccupiedSpace = -1;
                            }
                            break;
                        }
                    }
                }
            }

            if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
                sf::Vector2i mpix = mouseMoved->position;
                sf::Vector2f mp = window.mapPixelToCoords(mpix);

                if (grabbedIndex >= 0 && grabbedPlayer == currentPlayer) {
                    int   highlightIndex = -1;
                    float bestDist = 1e9f;

                    for (int i = 0; i < NUM_SPACES; ++i) {
                        sf::Vector2f c = spaces[i].getCenter();
                        float dx = mp.x - c.x;
                        float dy = mp.y - c.y;
                        float dist = std::sqrt(dx * dx + dy * dy);
                        if (dist < bestDist && dist < 80.f) {
                            bestDist = dist;
                            highlightIndex = i;
                        }
                    }

                    for (int i = 0; i < NUM_SPACES; ++i) {
                        if (i == highlightIndex && spaces[i].occupantPlayer == -1)
                            spaces[i].setHighlight(true);
                        else
                            spaces[i].setHighlight(false);
                    }

                    Tile& t = racks[grabbedPlayer][grabbedIndex];
                    t.setPosition(mp - t.grabOffset);
                }
                else {
                    for (auto& sp : spaces)
                        sp.setHighlight(false);
                }
            }

            if (const auto* mouseButtonReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (mouseButtonReleased->button != sf::Mouse::Button::Left)
                    continue;

                if (grabbedIndex >= 0 && grabbedPlayer == currentPlayer) {
                    Tile& t = racks[grabbedPlayer][grabbedIndex];
                    t.grabbed = false;

                    sf::Vector2f tileCenter = t.getCenter();
                    int   bestIdx = -1;
                    float bestDist = 1e9f;

                    for (int i = 0; i < NUM_SPACES; ++i) {
                        sf::Vector2f c = spaces[i].getCenter();
                        float dx = tileCenter.x - c.x;
                        float dy = tileCenter.y - c.y;
                        float dist = std::sqrt(dx * dx + dy * dy);
                        if (dist < bestDist && dist < 50.f) {
                            bestDist = dist;
                            bestIdx = i;
                        }
                    }

                    for (auto& sp : spaces)
                        sp.setHighlight(false);

                    if (bestIdx != -1 && spaces[bestIdx].occupantPlayer == -1) {
                        sf::Vector2f pos =
                            spaces[bestIdx].getCenter() - t.getSize() / 2.f;
                        t.setPosition(pos);
                        t.occupantSpace = bestIdx;

                        spaces[bestIdx].occupantPlayer = grabbedPlayer;
                        spaces[bestIdx].occupantIndex = grabbedIndex;

                        if (selectedButton >= 0 &&
                            selectedButton < static_cast<int>(buttons.size()))
                        {
                            Mult m = Mult::NONE;
                            if (selectedButton == 0)      m = Mult::DOUBLE_LETTER;
                            else if (selectedButton == 1) m = Mult::TRIPLE_LETTER;
                            else if (selectedButton == 2) m = Mult::DOUBLE_WORD;
                            else if (selectedButton == 3) m = Mult::TRIPLE_WORD;

                            spaces[bestIdx].mult = m;
                            spaces[bestIdx].applyMultiplierColorOrDefault();

                            selectedButton = -1;
                            for (auto& b : buttons) b.setPressed(false);
                        }
                    }
                    else {
                        if (prevOccupiedSpace != -1 &&
                            spaces[prevOccupiedSpace].occupantPlayer == -1)
                        {
                            sf::Vector2f pos =
                                spaces[prevOccupiedSpace].getCenter() - t.getSize() / 2.f;
                            t.setPosition(pos);
                            t.occupantSpace = prevOccupiedSpace;

                            spaces[prevOccupiedSpace].occupantPlayer = grabbedPlayer;
                            spaces[prevOccupiedSpace].occupantIndex = grabbedIndex;
                        }
                        else {
                            t.setPosition(t.revertPosition);
                            t.occupantSpace = -1;
                        }
                    }
                    grabbedIndex = -1;
                    grabbedPlayer = -1;
                    prevOccupiedSpace = -1;
                }
            }
        }
        int currentPlacedScore = computePlacedScoreForPlayer(currentPlayer);

        const float maxVisualScore = 50.f;
        float fillRatio = clampFloat(
            static_cast<float>(currentPlacedScore) / maxVisualScore,
            0.f, 1.f
        );

        sf::Vector2f bgPos = barBg.getPosition();
        sf::Vector2f bgSize = barBg.getSize();

        barFill.setPosition(bgPos);
        barFill.setSize(sf::Vector2f(bgSize.x * fillRatio, barHeight));

        scoreLabel.setString("Score (this move): " + std::to_string(currentPlacedScore));
        {
            sf::FloatRect sb = scoreLabel.getLocalBounds();
            scoreLabel.setPosition(
                bgPos + sf::Vector2f(
                    12.f,
                    (barHeight - sb.size.y) / 2.f - sb.position.y
                )
            );
        }

        totalLabelP0.setString("P1 Total: " + std::to_string(totals[0]));
        totalLabelP0.setPosition(bgPos + sf::Vector2f(12.f, -28.f));

        totalLabelP1.setString("P2 Total: " + std::to_string(totals[1]));
        totalLabelP1.setPosition(bgPos + sf::Vector2f(bgSize.x - 140.f, -28.f));

        turnLabel.setString("Turn: Player " + std::to_string(currentPlayer + 1));
        turnLabel.setPosition(bgPos + sf::Vector2f(bgSize.x / 2.f - 60.f, -28.f));

        movesLabel.setString(
            "Moves: " + std::to_string(movesDone) + " / " + std::to_string(MAX_MOVES)
        );
        movesLabel.setPosition(
            bgPos + sf::Vector2f(bgSize.x - 200.f, (barHeight - 20.f) / 2.f)
        );

        bagCountText.setString("Tiles left: " + std::to_string(static_cast<int>(bag.size())));
        bagCountText.setPosition(bgPos + sf::Vector2f(bgSize.x - 200.f, -5.f));

        std::string selText = (selectedButton == -1)
            ? "None"
            : ("#" + std::to_string(selectedButton + 1));

        help.setString(
            "Player " + std::to_string(currentPlayer + 1) +
            " turn. Left-click multiplier then drop tile. Right-click space to remove multiplier. Selected: " +
            selText
        );
        help.setPosition(sf::Vector2f(10.f, static_cast<float>(WINDOW_H) - 26.f));

        window.clear(sf::Color(30, 100, 40));

        window.draw(barBg);
        window.draw(barFill);
        window.draw(scoreLabel);
        window.draw(caption);
        window.draw(totalLabelP0);
        window.draw(totalLabelP1);
        window.draw(turnLabel);
        window.draw(movesLabel);
        window.draw(bagCountText);

        for (const Space& sp : spaces) sp.draw(window);
        for (const Tile& t : racks[0]) t.draw(window);
        for (const Tile& t : racks[1]) t.draw(window); 

        for (const Button& b : buttons) b.draw(window, font);
        commitBtn.draw(window, font);

        window.draw(help);

        if (isGameOver()) {
            std::string result;
            if (totals[0] > totals[1])      result = "Game over. Player 1 wins!";
            else if (totals[1] > totals[0]) result = "Game over. Player 2 wins!";
            else                             result = "Game over. It's a tie!";

            sf::RectangleShape overlay;
            overlay.setSize(sf::Vector2f(static_cast<float>(WINDOW_W) - 200.f, 140.f));
            overlay.setPosition(sf::Vector2f(100.f, static_cast<float>(WINDOW_H) / 2.f - 80.f));
            overlay.setFillColor(sf::Color(0, 0, 0, 200));
            overlay.setOutlineColor(sf::Color::White);
            overlay.setOutlineThickness(2.f);

            sf::Text resText(font);
            resText.setString(result);
            resText.setCharacterSize(28);
            resText.setFillColor(sf::Color::White);
            resText.setPosition(overlay.getPosition() + sf::Vector2f(20.f, 20.f));

            sf::Text finalScore(font);
            finalScore.setCharacterSize(20);
            finalScore.setFillColor(sf::Color::White);
            finalScore.setString(
                "Final - P1: " + std::to_string(totals[0]) +
                "   P2: " + std::to_string(totals[1])
            );
            finalScore.setPosition(overlay.getPosition() + sf::Vector2f(20.f, 60.f));

            window.draw(overlay);
            window.draw(resText);
            window.draw(finalScore);
        }

        window.display();
    }

    return 0;
}
