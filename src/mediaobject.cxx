module;

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QProcess>
#include <QtCore/qtmochelpers.h>
#include <phonon/AddonInterface>
#include <phonon/GlobalDescriptionContainer>
#include <phonon/MediaController>
#include <phonon/MediaObjectInterface>

#define ABOUT_TO_FINISH 2000
#define TO_MSEC 1000.0F
#define BASE10 10

export module phonon_native:mediaobject;

using Qt::Literals::StringLiterals::operator""_L1;

namespace Phonon::Native {
	class Backend;

	class MediaObject final:
		public QObject,
		public MediaObjectInterface,
		public AddonInterface {
		Q_OBJECT
		Q_INTERFACES(Phonon::MediaObjectInterface Phonon::AddonInterface)

	  public:
		explicit MediaObject(QObject* parent):
			QObject{parent},
			m_player{new QMediaPlayer{this}},
			m_process{new QProcess{this}} {
			connect(m_player,
				&QMediaPlayer::positionChanged,
				this,
				&MediaObject::timeChanged,
				Qt::AutoConnection);
			connect(m_player,
				&QMediaPlayer::hasVideoChanged,
				this,
				&MediaObject::hasVideoChanged,
				Qt::AutoConnection);
			connect(m_player,
				&QMediaPlayer::seekableChanged,
				this,
				&MediaObject::seekableChanged,
				Qt::AutoConnection);
			connect(m_player,
				&QMediaPlayer::mediaStatusChanged,
				this,
				&MediaObject::onMediaStatusChanged,
				Qt::AutoConnection);
			connect(
				m_player,
				&QMediaPlayer::bufferProgressChanged,
				this,
				[=, this](float progress) {
					emit bufferStatus(static_cast<int>(progress * 100.0F));
				},
				Qt::AutoConnection);
			connect(m_player,
				&QMediaPlayer::durationChanged,
				this,
				&MediaObject::totalTimeChanged,
				Qt::AutoConnection);
			connect(m_player,
				&QMediaPlayer::metaDataChanged,
				this,
				&MediaObject::onMetadataChanged,
				Qt::AutoConnection);
			connect(m_player,
				&QMediaPlayer::tracksChanged,
				this,
				&MediaObject::availableSubtitlesChanged,
				Qt::AutoConnection);
		}

		~MediaObject() final = default;

		MediaObject(const MediaObject&) = delete;
		MediaObject(MediaObject&&) = delete;
		auto operator=(const MediaObject&) -> MediaObject& = delete;
		auto operator=(MediaObject&&) -> MediaObject& = delete;

		auto play() -> void final {
			if(m_state == PausedState) {
				m_player->play();
				emit stateChanged(PlayingState, m_state);
				m_state = PlayingState;
			}
		}

		auto pause() -> void final {
			if(m_state == BufferingState || m_state == PlayingState) {
				m_player->pause();
				emit stateChanged(PausedState, m_state);
				m_state = PausedState;
			}
		}

		auto stop() -> void final {
			m_nextSource = {};
			m_player->stop();
			emit stateChanged(StoppedState, m_state);
			m_state = StoppedState;
		}

		auto seek(qint64 milliseconds) -> void final {
			m_player->setPosition(milliseconds);
			if(milliseconds < m_lastTick) {
				m_lastTick = milliseconds;
			}
		}

		[[nodiscard]]
		auto tickInterval() const -> qint32 final {
			return m_tickInterval;
		}

		auto setTickInterval(qint32 interval) -> void final {
			m_tickInterval = interval;
		}

		[[nodiscard]]
		auto hasVideo() const -> bool final {
			return m_player->hasVideo();
		}

		[[nodiscard]]
		auto isSeekable() const -> bool final {
			return m_player->isSeekable();
		}

		[[nodiscard]]
		auto currentTime() const -> qint64 final {
			return m_player->position();
		}

		[[nodiscard]]
		auto state() const -> State final {
			return m_state;
		}

		[[nodiscard]]
		auto errorString() const -> QString final {
			return {};
		}

		[[nodiscard]]
		auto errorType() const -> ErrorType final {
			return {};
		}

		[[nodiscard]]
		auto totalTime() const -> qint64 final {
			return m_player->duration();
		}

		[[nodiscard]]
		auto source() const -> MediaSource final {
			return m_mediaSource;
		}

		auto setSource(const MediaSource& source) -> void final {
			QByteArray url;
			switch(source.type()) {
				case MediaSource::Invalid:
					qDebug() << Q_FUNC_INFO
							 << "MediaSource Type is Invalid:" << source.type();
					break;
				case MediaSource::LocalFile:
				case MediaSource::Url:
					qDebug() << "MediaSource::Url:" << source.url();
					m_player->setSource(source.url());
					break;
				case MediaSource::Disc:
				case MediaSource::AudioVideoCapture:
				case MediaSource::CaptureDevice:
					qDebug() << Q_FUNC_INFO << "MediaSource is not supported";
					break;
				case MediaSource::Empty:
					qDebug() << Q_FUNC_INFO << "MediaSource is empty.";
					break;
				case MediaSource::Stream:
					break;
			}
			m_mediaSource = source;
			emit currentSourceChanged(m_mediaSource);
		}

		auto setNextSource(const MediaSource& source) -> void final {
			if(m_state == StoppedState) {
				setSource(source);
			} else {
				m_nextSource = source;
			}
		}

		[[nodiscard]]
		auto prefinishMark() const -> qint32 final {
			return m_prefinishMark;
		}

		auto setPrefinishMark(qint32 mark) -> void final {
			m_prefinishMark = mark;
		}

		[[nodiscard]]
		auto transitionTime() const -> qint32 final {
			return m_transitionTime;
		}

		auto setTransitionTime(qint32 time) -> void final {
			m_transitionTime = time;
		}

		[[nodiscard]]
		auto hasInterface(Interface /*iface*/) const -> bool final {
			return true;
		}

		auto interfaceCall(Interface iface, int command,
			const QList<QVariant>& arguments) -> QVariant final {
			switch(iface) {
				case NavigationInterface:
					switch(static_cast<AddonInterface::NavigationCommand>(
						command)) {
						case AddonInterface::availableMenus:
							return QVariant::fromValue(
								QList<MediaController::NavigationMenu>());
						case AddonInterface::setMenu:
							return true;
					}
				case ChapterInterface:
					switch(
						static_cast<AddonInterface::ChapterCommand>(command)) {
						case AddonInterface::availableChapters:
							return m_chapters.size();
						case AddonInterface::chapter:
							return m_currentChapter;
						case AddonInterface::setChapter:
							m_player->setPosition(static_cast<qint64>(
								m_chapters.at(arguments.first().toInt(nullptr))
									.first));
							return true;
					}
				case AngleInterface:
					switch(static_cast<AddonInterface::AngleCommand>(command)) {
						case AddonInterface::availableAngles:
						case AddonInterface::angle:
							return 0;
						case AddonInterface::setAngle:
							return true;
					}
				case TitleInterface:
					switch(static_cast<AddonInterface::TitleCommand>(command)) {
						case AddonInterface::availableTitles:
						case AddonInterface::title:
							return 1;
						case AddonInterface::setAutoplayTitles:
						case AddonInterface::setTitle:
							return true;
						case AddonInterface::autoplayTitles:
							return false;
					}
				case SubtitleInterface:
					switch(
						static_cast<AddonInterface::SubtitleCommand>(command)) {
						case AddonInterface::availableSubtitles:
							return QVariant::fromValue(
								GlobalSubtitles::instance()->listFor(this));
						case AddonInterface::currentSubtitle:
							return QVariant::fromValue(
								GlobalSubtitles::instance()->localIdFor(
									this, m_player->activeSubtitleTrack()));
						case AddonInterface::setCurrentSubtitle:
							qDebug() << arguments;
							m_player->setActiveSubtitleTrack(
								GlobalSubtitles::instance()->localIdFor(this,
									arguments.first()
										.value<SubtitleDescription>()
										.index()));
							qDebug() << "Current Sub "
									 << m_player->activeSubtitleTrack();
							return true;
						case AddonInterface::setCurrentSubtitleFile:
						case AddonInterface::setSubtitleAutodetect:
						case AddonInterface::setSubtitleEncoding:
						case AddonInterface::setSubtitleFont:
						case AddonInterface::subtitleAutodetect:
							return true;
						case AddonInterface::subtitleEncoding:
							return "UTF";
						case AddonInterface::subtitleFont:
							return "default";
					}
				case AudioChannelInterface:
					switch(static_cast<AddonInterface::AudioChannelCommand>(
						command)) {
						case AddonInterface::availableAudioChannels:
							return QVariant::fromValue(
								GlobalAudioChannels::instance()->listFor(this));
						case AddonInterface::currentAudioChannel:
							return QVariant::fromValue(
								GlobalAudioChannels::instance()->localIdFor(
									this, m_player->activeAudioTrack()));
						case AddonInterface::setCurrentAudioChannel:
							m_player->setActiveAudioTrack(
								GlobalAudioChannels::instance()->localIdFor(
									this,
									arguments.first()
										.value<AudioChannelDescription>()
										.index()));
							return true;
					}
			}

			return {};
		}

	  private slots:

		auto timeChanged(qint64 time) -> void {
			if(m_state == PlayingState || m_state == BufferingState
				|| m_state == PausedState) {
				if(m_tickInterval != 0
					&& (time + m_tickInterval >= m_lastTick)) {
					m_lastTick = time;
					emit tick(time);
				}
			}
			if(m_state == PlayingState || m_state == BufferingState) {
				if(time >= m_player->duration() - m_prefinishMark) {
					emit prefinishMarkReached(
						static_cast<qint32>(m_player->duration() - time));
				}
				if(m_player->duration() > 0
					&& time >= m_player->duration() - ABOUT_TO_FINISH) {
					emit aboutToFinish();
				}
				for(auto i{0}; i < m_chapters.size(); i++) {
					auto start{
						static_cast<qint64>(m_chapters[i].first * TO_MSEC)};
					auto end{
						static_cast<qint64>(m_chapters[i].second * TO_MSEC)};
					if(i != m_currentChapter && start <= time && time <= end) {
						m_currentChapter = i;
						emit chapterChanged(m_currentChapter);
						break;
					}
				}
			}
		}

		auto onMediaStatusChanged(QMediaPlayer::MediaStatus status) -> void {
			State newState{};
			switch(status) {
				case QMediaPlayer::NoMedia:
					newState = StoppedState;
					break;
				case QMediaPlayer::LoadedMedia:
					{
						if(m_state == PlayingState) {
							return;
						}
						emit availableAudioChannelsChanged();
						emit titleChanged(0);
						emit availableTitlesChanged(1);
						emit angleChanged(0);
						emit availableAnglesChanged(1);
						GlobalAudioChannels::instance()->clearListFor(this);
						auto tracks{m_player->audioTracks()};
						for(auto i{0}; i < tracks.size(); i++) {
							auto title{tracks[i][QMediaMetaData::Title]};
							GlobalAudioChannels::instance()->add(this,
								i,

								title.toString(),
								"");
						}
						GlobalSubtitles::instance()->clearListFor(this);
						auto subtitles{m_player->subtitleTracks()};
						for(auto i{0}; i < subtitles.size(); i++) {
							auto title{subtitles[i][QMediaMetaData::Title]};
							GlobalSubtitles::instance()->add(this,
								i,
								(title.toString().isEmpty()
										? "Subtitle "
											  + QString::number(i, BASE10)
										: title.toString()),
								"");
						}
						m_process->start("ffprobe",
							QStringList()
								<< "-i" << m_player->source().toLocalFile()
								<< "-show_chapters"
								<< "-print_format"
								<< "json",
							QIODeviceBase::ReadWrite);
						if(m_process->waitForFinished(-1)) {
							auto output{QJsonDocument::fromJson(
								m_process->readAllStandardOutput(), nullptr)
											.object()};
							if(output.contains("chapters")) {
								m_chapters.clear();
								for(auto chapter:
									output.value("chapters").toArray()) {
									m_chapters << QPair<float, float>{
										chapter.toObject()["start_time"]
											.toString({})
											.toFloat(nullptr),
										chapter.toObject()["end_time"]
											.toString({})
											.toFloat(nullptr)};
								}
								emit availableChaptersChanged(
									static_cast<int>(m_chapters.size()));
							}
						}
						m_lastTick = 0;
						newState = PausedState;
						break;
					}
				case QMediaPlayer::LoadingMedia:
					newState = LoadingState;
					break;
				case QMediaPlayer::StalledMedia:
					newState = BufferingState;
					break;
				case QMediaPlayer::BufferingMedia:
				case QMediaPlayer::BufferedMedia:
					if(m_state == BufferingState) {
						newState = PlayingState;
					} else {
						return;
					}
					break;
				case QMediaPlayer::EndOfMedia:
					emit finished();
					newState = StoppedState;
					m_lastTick = 0;
					break;
				case QMediaPlayer::InvalidMedia:
					newState = ErrorState;
					break;
			}
			emit stateChanged(newState, m_state);
			m_state = newState;
			if(status == QMediaPlayer::LoadedMedia) {
				play();
			}
		}

		auto onMetadataChanged() -> void {
			QMediaMetaData metadata{m_player->metaData()};
			QMultiMap<QString, QString> metaDataMap{
				{"ALBUM"_L1, metadata[QMediaMetaData::AlbumTitle].toString()},
				{"TITLE"_L1, metadata[QMediaMetaData::Title].toString()},
				{"ARTIST"_L1, metadata[QMediaMetaData::Author].toString()},
				{"DATE"_L1, metadata[QMediaMetaData::Date].toString()},
				{"GENRE"_L1, metadata[QMediaMetaData::Genre].toString()},
				{"TRACKNUMBER"_L1,
					metadata[QMediaMetaData::TrackNumber].toString()},
				{"DESCRIPTION"_L1,
					metadata[QMediaMetaData::Description].toString()},
				{"COPYRIGHT"_L1,
					metadata[QMediaMetaData::Copyright].toString()},
				{"URL"_L1, metadata[QMediaMetaData::Url].toString()}};

			emit metaDataChanged(metaDataMap);
		}

	  signals:
		auto stateChanged(State _t1, State _t2) -> void;
		auto tick(qint64 _t1) -> void;
		auto hasVideoChanged(bool _t1) -> void;
		auto seekableChanged(bool _t1) -> void;
		auto prefinishMarkReached(qint32 _t1) -> void;
		auto bufferStatus(int _t1) -> void;
		auto finished() -> void;
		auto aboutToFinish() -> void;
		auto totalTimeChanged(qint64 _t1) -> void;
		auto metaDataChanged(QMultiMap<QString, QString> _t1) -> void;
		auto currentSourceChanged(MediaSource _t1) -> void;
		auto chapterChanged(int _t1) -> void;
		auto availableChaptersChanged(int _t1) -> void;
		auto availableSubtitlesChanged() -> void;
		auto availableAudioChannelsChanged() -> void;
		auto titleChanged(int _t1) -> void;
		auto availableTitlesChanged(int _t1) -> void;
		auto angleChanged(int _t1) -> void;
		auto availableAnglesChanged(int _t1) -> void;

	  private:
		QMediaPlayer* m_player{};
		QProcess* m_process{};
		MediaSource m_nextSource;
		MediaSource m_mediaSource;
		Phonon::State m_state{};
		qint32 m_prefinishMark{};
		qint32 m_tickInterval{};
		qint32 m_transitionTime{};
		qint64 m_lastTick{};
		QList<QPair<float, float>> m_chapters;
		int m_currentChapter{};
		int m_angle{};
		friend Backend;
	};
} // namespace Phonon::Native

#include "mediaobject.moc"
