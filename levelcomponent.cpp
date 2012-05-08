#include "data.h"

#include "levelcomponent.h"




LevelComponent::LevelComponent(QDeclarativeItem *parent)
  : QDeclarativeItem (parent)
{
}

LevelComponent::~LevelComponent()
{
}


void LevelComponent::load(int level)
{
    m_mevel = FlashbackData::Level::load(level);
    emit initialRoomChanged();
}


int LevelComponent::initialRoom() const
{
    return m_mevel.isNull() ? -1 : m_mevel->initialRoom();
}


int LevelComponent::adjacentRoomAt(int room, Direction dir)
{
    if (m_mevel.isNull())
        return -1;

    const auto &adjacentRooms = m_mevel->adjacentRooms();
    if (adjacentRooms.isEmpty() || adjacentRooms.size() <= room + dir)
        return -1;

    const auto res = adjacentRooms[room + dir];

    auto back = (dir == Direction::UP)   ? Direction::DOWN :
                (dir == Direction::DOWN) ? Direction::UP :
                (dir == Direction::LEFT) ? Direction::RIGHT :
                                           Direction::LEFT;

    if (res < 0 || adjacentRooms.size() <= res + back)
        return -1;

    if (room != adjacentRooms[res + back])
        return -1;

    return res;
}
