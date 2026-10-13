/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "settings/sections/settings_mixingram.h"

#include "core/file_utilities.h"
#include "data/data_wall_paper.h"
#include "lang/lang_keys.h"
#include "settings/sections/settings_main.h"
#include "settings/settings_builder.h"
#include "settings/settings_common_session.h"
#include "ui/image/image_prepare.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/wrap/vertical_layout.h"
#include "window/themes/window_theme.h"
#include "window/window_session_controller.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"

namespace Settings {
namespace {

using namespace Builder;

void ChooseMixinGramBackground(
		not_null<Window::SessionController*> controller,
		not_null<QWidget*> parent) {
	auto filters = QStringList{
		u"Файлы фона чата (*.gif *.png)"_q,
		u"GIF анимация (*.gif)"_q,
		u"PNG изображение (*.png)"_q,
		FileDialog::AllFilesFilter(),
	};
	const auto callback = crl::guard(controller, [=](
			const FileDialog::OpenResult &result) {
		if (result.paths.isEmpty() && result.remoteContent.isEmpty()) {
			return;
		}
		const auto filePath = result.paths.isEmpty()
			? QString()
			: result.paths.front();
		const auto isGif = filePath.endsWith(
			u".gif"_q,
			Qt::CaseInsensitive);
		if (isGif) {
			if (Window::Theme::Background()->setAnimatedGifFromFile(
					filePath,
					result.remoteContent)) {
				controller->showToast(
					u"Анимированный GIF-фон чата установлен!"_q);
				return;
			}
		}

		auto image = Images::Read({
			.path = filePath,
			.content = result.remoteContent,
			.forceOpaque = true,
		}).image;
		if (image.isNull() || image.width() <= 0 || image.height() <= 0) {
			return;
		}
		Window::Theme::Background()->clearAnimatedGif();
		Window::Theme::Background()->set(
			Data::CustomWallPaper(),
			std::move(image));
		Window::Theme::Background()->setTile(false);
		controller->showToast(u"Кастомный фон чата установлен!"_q);
	});
	FileDialog::GetOpenPath(
		parent.get(),
		u"Выберите GIF или PNG для фона чата"_q,
		filters.join(u";;"_q),
		crl::guard(parent, callback));
}

void BuildCustomBackgroundSection(SectionBuilder &builder) {
	builder.addSkip();
	builder.addSubsectionTitle({
		.id = u"mixingram/modules"_q,
		.title = rpl::single(u"Модули MixinGram"_q),
		.keywords = { u"mixingram"_q, u"custom"_q, u"gif"_q, u"фон"_q },
	});

	const auto controller = builder.controller();
	auto statusLabel = rpl::single(
		rpl::empty
	) | rpl::then(
		rpl::merge(
			Window::Theme::Background()->updates() | rpl::to_empty,
			Window::Theme::Background()->gifFrameUpdates()
		)
	) | rpl::map([] {
		const auto bg = Window::Theme::Background();
		if (bg->hasAnimatedGif()) {
			return u"GIF (анимированный)"_q;
		} else if (Data::IsCustomWallPaper(bg->paper())) {
			return u"PNG"_q;
		}
		return u"GIF / PNG"_q;
	});

	const auto button = builder.addButton({
		.id = u"mixingram/custom-background"_q,
		.title = rpl::single(u"Кастомный фон чата"_q),
		.icon = MixinGramWallpaperIconDescriptor(),
		.label = std::move(statusLabel),
		.keywords = {
			u"gif"_q,
			u"png"_q,
			u"wallpaper"_q,
			u"background"_q,
			u"фон"_q,
			u"анимация"_q,
		},
	});
	if (button) {
		button->addClickHandler([=] {
			ChooseMixinGramBackground(controller, button);
		});
	}

	builder.addSkip();
	builder.addDividerText(rpl::single(
		u"Нажмите «Кастомный фон чата», чтобы выбрать файл .gif или .png в проводнике. Выбранный файл сразу установится как фон чатов (с полной поддержкой анимированных GIF)."_q));
}

class MixinGram final : public Section<MixinGram> {
public:
	MixinGram(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

const auto kMeta = BuildHelper({
	.id = MixinGram::Id(),
	.parentId = MainId(),
	.title = &tr::lng_settings_mixingram,
	.icon = &st::menuIconMixinGram,
}, [](SectionBuilder &builder) {
	BuildCustomBackgroundSection(builder);
});

const SectionBuildMethod kMixinGramSection = kMeta.build;

MixinGram::MixinGram(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> MixinGram::title() {
	return rpl::single(u"Настройки MixinGram"_q);
}

void MixinGram::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

	build(content, kMixinGramSection);

	Ui::ResizeFitChild(this, content);
}

} // namespace

Type MixinGramId() {
	return MixinGram::Id();
}

} // namespace Settings
