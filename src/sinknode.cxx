module;

#include <QMediaPlayer>

export module phonon_native:sinknode;

namespace Phonon::Native {
	class SinkNode {
	  public:
		SinkNode() = default;
		SinkNode(const SinkNode&) = delete;
		SinkNode(SinkNode&&) = delete;
		auto operator=(const SinkNode&) -> SinkNode& = delete;
		auto operator=(SinkNode&&) -> SinkNode& = delete;
		virtual ~SinkNode() = default;

		virtual auto connectToMediaPlayer(QMediaPlayer* player) -> void {
			m_player = player;
		}

		virtual auto disconnectFromMediaPlayer(
			QMediaPlayer* /*mediaObject*/) -> void {
			m_player = nullptr;
		}

		auto mediaPlayer() -> QMediaPlayer* {
			return m_player;
		}

	  private:
		QMediaPlayer* m_player{};
	};
} // namespace Phonon::Native
