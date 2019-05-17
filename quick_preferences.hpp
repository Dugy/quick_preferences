/*
* \brief Class convenient and duplication-free serialisation and deserialisation of its descendants into JSON, with one function to handle both loading and saving
*
* See more at https://github.com/Dugy/quick_preferences
*/

#ifndef QUICK_PREFERENCES_HPP
#define QUICK_PREFERENCES_HPP

#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <memory>
#include <fstream>
#include <exception>
#include <sstream>
#include <type_traits>

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QObject>
#include <QPushButton>
#include <QGroupBox>

class QuickPreferences {

public:
	enum class JSONtype : uint8_t {
		NIL,
		STRING,
		NUMBER,
		BOOL,
		ARRAY,
		OBJECT
	};
	struct JSON {
		inline virtual JSONtype type() {
			return JSONtype::NIL;
		}
		inline virtual std::string& getString() {
			throw(std::runtime_error("String value is not really string"));
		}
		inline virtual double& getDouble() {
			throw(std::runtime_error("Double value is not really double"));
		}
		inline virtual bool& getBool() {
			throw(std::runtime_error("Bool value is not really bool"));
		}
		inline virtual std::vector<std::shared_ptr<JSON>>& getVector() {
			throw(std::runtime_error("Array value is not really array"));
		}
		inline virtual std::unordered_map<std::string, std::shared_ptr<JSON>>& getObject() {
			throw(std::runtime_error("Object value is not really an object"));
		}
		inline virtual void write(std::ostream& out, int = 0) {
			out << "null";
		}
		inline void writeToFile(const std::string& fileName) {
			std::ofstream out(fileName);
			if (!out.good()) throw(std::runtime_error("Could not write to file " + fileName));
			this->write(out, 0);
		}

		virtual ~JSON() = default;
	protected:
		static void writeString(std::ostream& out, const std::string& written) {
			out.put('"');
			for (unsigned int i = 0; i < written.size(); i++) {
				if (written[i] == '"') {
					out.put('/');
					out.put('"');
				} else if (written[i] == '\n') {
					out.put('\\');
					out.put('n');
				} else if (written[i] == '\\') {
					out.put('\\');
					out.put('\\');
				} else
					out.put(written[i]);
			}
			out.put('"');
		}
		static void indent(std::ostream& out, int depth) {
			for (int i = 0; i < depth; i++)
				out.put('\t');
		}
	};
	struct JSONstring : public JSON {
		std::string contents_;
		JSONstring(const std::string& from = "") : contents_(from) {}

		inline virtual JSONtype type() {
			return JSONtype::STRING;
		}
		inline virtual std::string& getString() {
			return contents_;
		}
		inline void write(std::ostream& out, int = 0) {
			writeString(out, contents_);
		}
	};
	struct JSONdouble : public JSON {
		double value_;
		JSONdouble(double from = 0) : value_(from) {}

		inline virtual JSONtype type() {
			return JSONtype::NUMBER;
		}
		inline virtual double& getDouble() {
			return value_;
		}
		inline void write(std::ostream& out, int = 0) {
			out << value_;
		}
	};
	struct JSONbool : public JSON {
		bool value_;
		JSONbool(bool from = false) : value_(from) {}

		inline virtual JSONtype type() {
			return JSONtype::BOOL;
		}
		inline virtual bool& getBool() {
			return value_;
		}
		inline void write(std::ostream& out, int = 0) {
			out << (value_ ? "true" : "false");
		}
	};
	struct JSONobject : public JSON {
		std::unordered_map<std::string, std::shared_ptr<JSON>> contents_;
		JSONobject() {}

		inline virtual JSONtype type() {
			return JSONtype::OBJECT;
		}
		inline virtual std::unordered_map<std::string, std::shared_ptr<JSON>>& getObject() {
			return contents_;
		}
		inline void write(std::ostream& out, int depth = 0) {
			if (contents_.empty()) {
				out.put('{');
				out.put('}');
				return;
			}
			out.put('{');
			out.put('\n');
			bool first = true;
			for (auto& it : contents_) {
				if (first)
					first = false;
				else {
					out.put(',');
					out.put('\n');
				}
				indent(out, depth + 1);
				writeString(out, it.first);
				out.put(':');
				out.put(' ');
				it.second->write(out, depth + 1);
			}
			out.put('\n');
			indent(out, depth);
			out.put('}');
		}
	};
	struct JSONarray : public JSON {
		std::vector<std::shared_ptr<JSON>> contents_;
		JSONarray() {}

		inline virtual JSONtype type() {
			return JSONtype::ARRAY;
		}
		inline virtual std::vector<std::shared_ptr<JSON>>& getVector() {
			return contents_;
		}
		inline void write(std::ostream& out, int depth = 0) {
			out.put('[');
			if (contents_.empty()) {
				out.put(']');
				return;
			}
			for (auto& it : contents_) {
				out.put('\n');
				indent(out, depth);
				indent(out, depth);
				it->write(out, depth + 1);
			}
			out.put('\n');
			indent(out, depth);
			out.put(']');
			
		}
	};

	static std::shared_ptr<JSON> parseJSON(std::istream& in) {
		auto readString = [&in] () -> std::string {
			char letter = in.get();
			std::string collected;
			while (letter != '"') {
				if (letter == '\\') {
					if (in.get() == '"') collected.push_back('"');
					else if (in.get() == 'n') collected.push_back('\n');
					else if (in.get() == '\\') collected.push_back('\\');
				} else {
					collected.push_back(letter);
				}
				letter = in.get();
			}
			return collected;
		};
		auto readWhitespace = [&in] () -> char {
			char letter;
			do {
				letter = in.get();
			} while (letter == ' ' || letter == '\t' || letter == '\n' || letter == ',');
			return letter;
		};

		char letter = readWhitespace();
		if (letter == 0 || letter == EOF) return std::make_shared<JSON>();
		else if (letter == '"') {
			return std::make_shared<JSONstring>(readString());
		}
		else if (letter == 't') {
			if (in.get() == 'r' && in.get() == 'u' && in.get() == 'e')
				return std::make_shared<JSONbool>(true);
			else
				throw(std::runtime_error("JSON parser found misspelled bool 'true'"));
		}
		else if (letter == 'f') {
			if (in.get() == 'a' && in.get() == 'l' && in.get() == 's' && in.get() == 'e')
				return std::make_shared<JSONbool>(false);
			else
				throw(std::runtime_error("JSON parser found misspelled bool 'false'"));
		}
		else if (letter == 'n') {
			if (in.get() == 'u' && in.get() == 'l' && in.get() == 'l')
				return std::make_shared<JSON>();
			else
				throw(std::runtime_error("JSON parser found misspelled bool 'null'"));
		}
		else if (letter == '-' || (letter >= '0' && letter <= '9')) {
			std::string asString;
			asString.push_back(letter);
			do {
				letter = in.get();
				asString.push_back(letter);
			} while (letter == '-' || letter == 'E' || letter == 'e' || letter == ',' || letter == '.' || (letter >= '0' && letter <= '9'));
			in.unget();
			std::stringstream parsing(asString);
			double number;
			parsing >> number;
			return std::make_shared<JSONdouble>(number);
		}
		else if (letter == '{') {
			auto retval = std::make_shared<JSONobject>();
			do {
				letter = readWhitespace();
				if (letter == '"') {
					const std::string& name = readString();
					letter = readWhitespace();
					if (letter != ':') throw(std::runtime_error("JSON parser expected an additional ':' somewhere"));
					retval->getObject()[name] = parseJSON(in);
				} else break;
			} while (letter != '}');
			return retval;
		}
		else if (letter == '[') {
			auto retval = std::make_shared<JSONarray>();
			do {
				letter = readWhitespace();
				if (letter == '{') {
					in.unget();
					retval->getVector().push_back(parseJSON(in));
				} else break;
			} while (letter != ']');
			return retval;
		} else {
			throw(std::runtime_error("JSON parser found unexpected character " + letter));
		}
		return std::make_shared<JSON>();
	}
	static std::shared_ptr<JSON> parseJSON(const std::string& fileName) {
		std::ifstream in(fileName);
		if (!in.good()) return std::make_shared<JSON>();
		return parseJSON(in);
	}
protected:

	enum class ActionType : uint8_t {
		LOADING,
		SAVING,
		GUI,
		GUItable
	};

private:

	struct GUImakingInfo {
		QGridLayout* layout = nullptr;
		int gridDown = 0;
		int gridRight = 0;
		std::shared_ptr<std::function<void()>> callback;
	};
	mutable union {
		JSON* preferencesJson;
		GUImakingInfo* guiInfo;
	} actionData_;
	mutable ActionType action_;

	void placeTableWidget(QWidget* placed, const std::string& title) {
		if (actionData_.guiInfo->gridDown == 1)
			actionData_.guiInfo->layout->addWidget(new QLabel(QString::fromStdString(title)), 0, actionData_.guiInfo->gridRight);
		actionData_.guiInfo->layout->addWidget(placed, actionData_.guiInfo->gridDown, actionData_.guiInfo->gridRight);
		actionData_.guiInfo->gridRight++;
	}

protected:
	/*!
	* \brief Should all the synch() method on all members that are to be saved
	*
	* \note If something unusual needs to be done, the saving() method returns if it's being saved or not
	* \note The sync() method knows whether to save it or not
	* \note Const correctness is violated here, but I find it better than massive duplication
	*/
	virtual void process() = 0;

	/*!
	* \brief Returns if the structure is being saved or loaded
	*
	* \return Whether it's saved (loaded if false)
	*
	* \note Result is meaningless outside a process() overload
	*/
	inline ActionType action() {
		return action_;
	}

	/*!
	* \brief Prepares internal parameters for the synch() calls made by process()
	* \param The layout it should fill
	* \param The vertical offset in the layout grid
	* \param The horizontal offset in the layout grid
	* \param A shared pointer to a callback function
	*
	* \note This is useful when overloading constructGUI()
	*/
	inline void setupProcess(QGridLayout* layout, int gridDown = 0, int gridRight = 0, std::shared_ptr<std::function<void()>> callback = nullptr) {
		actionData_.guiInfo->gridDown = gridDown;
		actionData_.guiInfo->gridRight = gridRight;
		actionData_.guiInfo->layout = layout;
		actionData_.guiInfo->callback = callback;
	}

	/*!
	* \brief Saves or loads a string value
	* \param The name of the value in the output/input file
	* \param Reference to the value
	* \return false if the value was absent while reading, true otherwise
	*/
	inline bool synch(const std::string& key, std::string& value) {
		switch (action_) {
		case ActionType::SAVING:
			actionData_.preferencesJson->getObject()[key] = std::make_shared<JSONstring>(value);
			return true;
		case ActionType::LOADING:
		{
			auto found = actionData_.preferencesJson->getObject().find(key);
			if (found != actionData_.preferencesJson->getObject().end()) {
				value = found->second->getString();
				return true;
			} else return false;
		}
		case ActionType::GUI:
			actionData_.guiInfo->layout->addWidget(new QLabel(QString::fromStdString(key + ":")), actionData_.guiInfo->gridDown, 0);
		case ActionType::GUItable:
			QLineEdit* editor = new QLineEdit(QString::fromStdString(value));
			std::shared_ptr<std::function<void()>> callback = actionData_.guiInfo->callback;
			QObject::connect(editor, &QLineEdit::editingFinished, actionData_.guiInfo->layout, [&value, editor, callback]() {
				value = editor->text().toStdString();
				if (callback) (*callback)();
			});
			if (action_ == ActionType::GUI) {
				actionData_.guiInfo->layout->addWidget(editor, actionData_.guiInfo->gridDown, 1);
				actionData_.guiInfo->gridDown++;
			}
			else placeTableWidget(editor, key);
			return true;
		}
	}
	
	/*!
	* \brief Saves or loads an arithmetic value
	* \param The name of the value in the output/input file
	* \param Reference to the value
	* \return false if the value was absent while reading, true otherwise
	*
	* \note The value is converted from and to a double for JSON conformity
	*/
	template<typename T>
	typename std::enable_if<std::is_arithmetic<T>::value && !std::is_same<T, bool>::value, bool>::type
	synch(const std::string& key, T& value) {
		switch (action_) {
		case ActionType::SAVING:
			actionData_.preferencesJson->getObject()[key] = std::make_shared<JSONdouble>(double(value));
			return true;
		case ActionType::LOADING:
		{
			auto found = actionData_.preferencesJson->getObject().find(key);
			if (found != actionData_.preferencesJson->getObject().end()) {
				value = T(found->second->getDouble());
				return true;
			} return false;
		}
		case ActionType::GUI:
			actionData_.guiInfo->layout->addWidget(new QLabel(QString::fromStdString(key + ":")), actionData_.guiInfo->gridDown, 0);
		case ActionType::GUItable:
			QLineEdit* editor = new QLineEdit(QString::fromStdString(std::to_string(value)));
			std::shared_ptr<std::function<void()>> callback = actionData_.guiInfo->callback;
			QObject::connect(editor, &QLineEdit::editingFinished, actionData_.guiInfo->layout, [&value, editor, callback]() {
				if (std::is_integral<T>::value)
					value = editor->text().toLong();
				else
					value = editor->text().toDouble();
				if (callback) (*callback)();
			});
			if (action_ == ActionType::GUI) {
				actionData_.guiInfo->layout->addWidget(editor, actionData_.guiInfo->gridDown, 1);
				actionData_.guiInfo->gridDown++;
			}
			else placeTableWidget(editor, key);
			return true;
		}
	}

	/*!
	* \brief Saves or loads a bool
	* \param The name of the value in the output/input file
	* \param Reference to the value
	* \return false if the value was absent while reading, true otherwise
	*/
	inline bool synch(const std::string& key, bool& value) {
		switch (action_) {
		case ActionType::SAVING:
			actionData_.preferencesJson->getObject()[key] = std::make_shared<JSONbool>(value);
			return true;
		case ActionType::LOADING:
		{
			auto found = actionData_.preferencesJson->getObject().find(key);
			if (found != actionData_.preferencesJson->getObject().end()) {
				value = found->second->getBool();
				return true;
			} else return false;
		}
		case ActionType::GUI:
		case ActionType::GUItable:
			QCheckBox* check = new QCheckBox(QString::fromStdString((action_ == ActionType::GUI) ? key : ""));
			check->setChecked(value);
			std::shared_ptr<std::function<void()>> callback = actionData_.guiInfo->callback;
			QObject::connect(check, &QCheckBox::clicked, actionData_.guiInfo->layout, [&value, check, callback]() {
				value = check->isChecked();
				if (callback) (*callback)();
			});
			if (action_ == ActionType::GUI) {
				actionData_.guiInfo->layout->addWidget(check, actionData_.guiInfo->gridDown, 0, 1, 2);
				actionData_.guiInfo->gridDown++;
			}
			else placeTableWidget(check, key);
			return true;
		}
	}
	
	/*!
	* \brief Saves or loads an object derived from QuickPreferences held in a smart pointer
	* \param The name of the value in the output/input file
	* \param Reference to the pointer
	* \return false if the value was absent while reading, true otherwise
	*
	\ \note The smart pointer class must be dereferencable through operator*(), constructible from raw pointer to the class and the ! operation must result in a number
	* \note If not null, the contents will be overwritten, so raw pointers must be initalised before calling it, but no memory leak will occur
	*/
	template<typename T>
	typename std::enable_if<!std::is_base_of<QuickPreferences, T>::value
			&& std::is_same<bool, typename std::remove_reference<decltype(std::declval<QuickPreferences>().synch(std::declval<std::string>(), *std::declval<T>()))>::type>::value
			&& std::is_constructible<T, typename std::remove_reference<decltype(*std::declval<T>())>::type*>::value
			&& std::is_arithmetic<typename std::remove_reference<decltype(!std::declval<T>())>::type>::value , bool>::type
	synch(const std::string& key, T& value) {
		switch (action_) {
		case ActionType::SAVING:
			if (!value)
				actionData_.preferencesJson->getObject()[key] = std::make_shared<JSON>();
			else {
				synch(key, *value);
			}
			return true;
		case ActionType::LOADING:
		{
			auto found = actionData_.preferencesJson->getObject().find(key);
			if (found != actionData_.preferencesJson->getObject().end()) {
				if (found->second->type() != JSONtype::NIL) {
					value = T(new typename std::remove_reference<decltype(*std::declval<T>())>::type());
					synch(key, *value);
				} else
					value = nullptr;
				return true;
			} else {
				value = nullptr;
				return false;
			}
		}
		case ActionType::GUI:
		case ActionType::GUItable:
			QGroupBox* group = new QGroupBox(QString::fromStdString((action_ == ActionType::GUI) ? key : ""));
			group->setCheckable(true);
			group->setChecked(bool(value));
			actionData_.guiInfo->layout->addWidget(group, actionData_.guiInfo->gridDown, 0, 1, 2);
			group->setLayout(new QGridLayout);
			int regularMargin = group->layout()->margin();
			group->layout()->setMargin(0);
			std::shared_ptr<std::function<void()>> callback = actionData_.guiInfo->callback;
			auto fill = [&value, regularMargin, group, callback] () {
				group->layout()->setMargin(regularMargin);
				value->makeGUI(static_cast<QGridLayout*>(group->layout()), 0, 0, callback);
			};
			if (value)
				fill();
			QObject::connect(group, &QGroupBox::clicked, group, [&value, group, fill, callback]() {
				if (group->isChecked()) {
					value = T(new typename std::remove_reference<decltype(*std::declval<T>())>::type());
					fill();
				} else {
					qDeleteAll(group->children());
					delete group->layout();
					group->setLayout(new QGridLayout);
					group->layout()->setMargin(0);
					value = nullptr;
				}
				if (callback) (*callback)();
			});
			if (action_ == ActionType::GUI) {
				actionData_.guiInfo->layout->addWidget(group, actionData_.guiInfo->gridDown, 0, 1, 2);
				actionData_.guiInfo->gridDown++;
			}
			else placeTableWidget(group, key);
			return true;
		}
	}
	
	/*!
	* \brief Saves or loads an object derived from QuickPreferences
	* \param The name of the value in the output/input file
	* \param Reference to the value
	* \return false if the value was absent while reading, true otherwise
	*/
	template<typename T>
	typename std::enable_if<std::is_base_of<QuickPreferences, T>::value, bool>::type
	synch(const std::string& key, T& value) {
		value.action_ = action_;
		switch (action_) {
		case ActionType::SAVING:
		{
			auto making = std::make_shared<JSONobject>();
			value.actionData_.preferencesJson = making.get();
			value.process();
			actionData_.preferencesJson->getObject()[key] = making;
			value.actionData_.preferencesJson = nullptr;
			return true;
		}
		case ActionType::LOADING:
		{
			auto found = actionData_.preferencesJson->getObject().find(key);
			if (found != actionData_.preferencesJson->getObject().end()) {
				value.actionData_.preferencesJson = found->second.get();
				value.process();
				value.actionData_.preferencesJson = nullptr;
				return true;
			} else return false;
		}
		case ActionType::GUI:
		case ActionType::GUItable:
			QGroupBox* group = new QGroupBox(QString::fromStdString(key + ":"));
			QVBoxLayout* subLayout = new QVBoxLayout();
			group->setLayout(subLayout);
			QFrame* innerFrame = new QFrame;
			QGridLayout* innerLayout = new QGridLayout;
			innerFrame->setLayout(innerLayout);
			subLayout->addWidget(innerFrame);
			value.makeGUI(innerLayout, 0, 0, actionData_.guiInfo->callback);
			if (action_ == ActionType::GUI) {
				actionData_.guiInfo->layout->addWidget(group, actionData_.guiInfo->gridDown, 0, 1, 2);
				actionData_.guiInfo->gridDown++;
			}
			else placeTableWidget(group, key);
			return true;
		}
	}
	
	/*!
	* \brief Saves or loads a vector of objects derived from QuickPreferences
	* \param The name of the value in the output/input file
	* \param Reference to the vector
	* \return false if the value was absent while reading, true otherwise
	*
	* \note Class must be default constructible
	*/
	template<typename T>
	typename std::enable_if<std::is_base_of<QuickPreferences, T>::value, bool>::type
	synch(const std::string& key, std::vector<T>& value) {
		switch (action_) {
		case ActionType::SAVING:
		{
			auto making = std::make_shared<JSONarray>();
			for (unsigned int i = 0; i < value.size(); i++) {
				auto innerMaking = std::make_shared<JSONobject>();
				value[i].action_ = ActionType::SAVING;
				value[i].actionData_.preferencesJson = innerMaking.get();
				value[i].process();
				value[i].actionData_.preferencesJson = nullptr;
				making->getVector().push_back(innerMaking);
			}
			actionData_.preferencesJson->getObject()[key] = making;
			return true;
		}
		case ActionType::LOADING:
		{
			value.clear();
			auto found = actionData_.preferencesJson->getObject().find(key);
			if (found != actionData_.preferencesJson->getObject().end()) {
				for (unsigned int i = 0; i < found->second->getVector().size(); i++) {
					value.push_back(T());
					T& filled = value.back();
					filled.action_ = ActionType::LOADING;
					filled.actionData_.preferencesJson = found->second->getVector()[i].get();
					filled.process();
					filled.actionData_.preferencesJson = nullptr;
					filled.actionData_.preferencesJson = nullptr;
				}
				return true;
			} else return false;
		}
		case ActionType::GUI:
		{
			QWidget* group = new QGroupBox(QString::fromStdString(key + ":"));
			QVBoxLayout* subLayout = new QVBoxLayout();
			group->setLayout(subLayout);
			QFrame* innerFrame = new QFrame;
			subLayout->addWidget(innerFrame);
			// Note about the strange shared_ptr to unique_ptr: the lambda cannot capture its own shared pointer or it will be never destroyed
			std::shared_ptr<std::unique_ptr<std::function<void()>>> regenerateTable = std::make_shared<std::unique_ptr<std::function<void()>>>();
			std::unique_ptr<std::function<void()>>* regenerateSafe = regenerateTable.get();
			std::shared_ptr<std::function<void()>> callback = actionData_.guiInfo->callback;
			(*regenerateSafe) = std::unique_ptr<std::function<void()>>(new std::function<void()>([innerFrame, &value, regenerateSafe, callback] () {
				// There is no function to clear everything in a layout
				if (innerFrame->layout()) {
					qDeleteAll(innerFrame->children());
					delete innerFrame->layout();
				}
				QGridLayout* innerLayout = new QGridLayout;
				innerFrame->setLayout(innerLayout);
				int subGridDown = 1;
				int deletePosition = 0;
				for (auto it = value.begin(); it != value.end(); ++it) {
					innerLayout->addWidget(new QLabel(QString::number(subGridDown)), subGridDown, 0);
					it->makeGUItable(innerLayout, subGridDown, 1, callback);
					QPushButton* deleteButton = new QPushButton(QPushButton::tr("Delete"));
					if (!deletePosition)
						deletePosition = innerLayout->columnCount();
					innerLayout->addWidget(deleteButton, subGridDown, deletePosition);
					QObject::connect(deleteButton, &QPushButton::clicked, deleteButton, [&value, it, regenerateSafe, callback]() {
						value.erase(it);
						if (callback) (*callback)();
						(**regenerateSafe)();
					});
					subGridDown++;
				}
			}));
			(**regenerateTable)();
			QPushButton* addButton = new QPushButton(QPushButton::tr("Add"));
			QObject::connect(addButton, &QPushButton::clicked, addButton, [&value, regenerateTable, callback]() {
				value.emplace_back();
				if (callback) (*callback)();
				(**regenerateTable)();
			});
			subLayout->addWidget(addButton);
			actionData_.guiInfo->layout->addWidget(group, actionData_.guiInfo->gridDown, 0, 1, 2);
			actionData_.guiInfo->gridDown++;
			return true;
		}
		case ActionType::GUItable:
			throw(std::logic_error("GUItable can't be called on vectors"));
		}
	}
	
	/*!
	* \brief Saves or loads a vector of smart pointers to objects derived from QuickPreferences
	* \param The name of the value in the output/input file
	* \param Reference to the vector
	* \return false if the value was absent while reading, true otherwise
	*
	* \note The smart pointer class must be dereferencable through operator*() and constructible from raw pointer to the class
	* \note The class must be default constructible
	* \note Using a vector of raw pointers may cause memory leaks if there is some content before loading
	*/
	template<typename T>
	typename std::enable_if<std::is_base_of<QuickPreferences, typename std::remove_reference<decltype(*std::declval<T>())>::type>::value
			&& std::is_constructible<T, typename std::remove_reference<decltype(*std::declval<T>())>::type*>::value, bool>::type
	synch(const std::string& key, std::vector<T>& value) {
		switch (action_) {
		case ActionType::SAVING:
		{
			auto making = std::make_shared<JSONarray>();
			for (unsigned int i = 0; i < value.size(); i++) {
				auto innerMaking = std::make_shared<JSONobject>();
				(*value[i]).action_ = ActionType::SAVING;
				(*value[i]).actionData_.preferencesJson = innerMaking.get();
				(*value[i]).process();
				(*value[i]).actionData_.preferencesJson = nullptr;
				making->getVector().push_back(innerMaking);
			}
			actionData_.preferencesJson->getObject()[key] = making;
			return true;
		}
		case ActionType::LOADING:
		{
			value.clear();
			auto found = actionData_.preferencesJson->getObject().find(key);
			if (found != actionData_.preferencesJson->getObject().end()) {
				for (unsigned int i = 0; i < found->second->getVector().size(); i++) {
					value.emplace_back(new typename std::remove_reference<decltype(*std::declval<T>())>::type());
					T& filled = value.back();
					(*value[i]).action_ = ActionType::LOADING;
					(*filled).actionData_.preferencesJson = found->second->getVector()[i].get();
					(*filled).process();
					(*filled).actionData_.preferencesJson = nullptr;
				}
				return true;
			} else return false;
		}
		case ActionType::GUI:
		{
			QWidget* group = new QGroupBox(QString::fromStdString(key + ":"));
			QVBoxLayout* subLayout = new QVBoxLayout();
			group->setLayout(subLayout);
			QFrame* innerFrame = new QFrame;
			subLayout->addWidget(innerFrame);
			// Note about the strange shared_ptr to unique_ptr: the lambda cannot capture its own shared pointer or it will be never destroyed
			std::shared_ptr<std::unique_ptr<std::function<void()>>> regenerateTable = std::make_shared<std::unique_ptr<std::function<void()>>>();
			std::unique_ptr<std::function<void()>>* regenerateSafe = regenerateTable.get();
			std::shared_ptr<std::function<void()>> callback = actionData_.guiInfo->callback;
			(*regenerateSafe) = std::unique_ptr<std::function<void()>>(new std::function<void()>([innerFrame, &value, regenerateSafe, callback] () {
				// There is no function to clear everything in a layout
				if (innerFrame->layout()) {
					qDeleteAll(innerFrame->children());
					delete innerFrame->layout();
				}
				QGridLayout* innerLayout = new QGridLayout;
				innerFrame->setLayout(innerLayout);
				int subGridDown = 1;
				int deletePosition = 0;
				for (auto it = value.begin(); it != value.end(); ++it) {
					innerLayout->addWidget(new QLabel(QString::number(subGridDown)), subGridDown, 0);
					(*it)->makeGUItable(innerLayout, subGridDown, 1, callback);
					QPushButton* deleteButton = new QPushButton(QPushButton::tr("Delete"));
					if (!deletePosition)
						deletePosition = innerLayout->columnCount();
					innerLayout->addWidget(deleteButton, subGridDown, deletePosition);
					QObject::connect(deleteButton, &QPushButton::clicked, deleteButton, [&value, it, regenerateSafe, callback]() {
						value.erase(it);
						if (callback) (*callback)();
						(**regenerateSafe)();
					});
					subGridDown++;
				}
			}));
			(**regenerateTable)();
			QPushButton* addButton = new QPushButton(QPushButton::tr("Add"));
			QObject::connect(addButton, &QPushButton::clicked, addButton, [&value, regenerateTable, callback]() {
				value.emplace_back(new typename std::remove_reference<decltype(*std::declval<T>())>::type());
				if (callback) (*callback)();
				(**regenerateTable)();
			});
			subLayout->addWidget(addButton);
			actionData_.guiInfo->layout->addWidget(group, actionData_.guiInfo->gridDown, 0, 1, 2);
			actionData_.guiInfo->gridDown++;
			return true;
		}
		case ActionType::GUItable:
			throw(std::logic_error("GUItable can't be called on vectors"));
		}
	}

public:
	/*!
	* \brief Serialises the object to a JSON string
	* \return The JSON string
	*
	* \note It calls the overloaded process() method
	* \note Not only that it's not thread-safe, it's not even reentrant
	*/
	inline std::string serialise() const {
		std::shared_ptr<JSON> target = std::make_shared<JSONobject>();
		actionData_.preferencesJson = target.get();
		action_ = ActionType::SAVING;
		const_cast<QuickPreferences*>(this)->process();
		std::stringstream out;
		actionData_.preferencesJson->write(out);
		actionData_.preferencesJson = nullptr;
		return out.str();
	}

	/*!
	* \brief Saves the object to a JSON file
	* \param The name of the JSON file
	*
	* \note It calls the overloaded process() method
	* \note Not only that it's not thread-safe, it's not even reentrant
	*/
	inline void save(const std::string& fileName) const {
		std::shared_ptr<JSON> target = std::make_shared<JSONobject>();
		actionData_.preferencesJson = target.get();
		action_ = ActionType::SAVING;
		const_cast<QuickPreferences*>(this)->process();
		actionData_.preferencesJson->writeToFile(fileName);
		actionData_.preferencesJson = nullptr;
	}

	/*!
	* \brief Loads the object from a JSON string
	* \param The JSON string
	*
	* \note It calls the overloaded process() method
	* \note If the string is blank, nothing is done
	* \note Not only that it's not thread-safe, it's not even reentrant
	*/
	inline void deserialise(const std::string& source) {
		std::stringstream sourceStream(source);
		std::shared_ptr<JSON> target = parseJSON(source);
		actionData_.preferencesJson = target.get();
		if (actionData_.preferencesJson->type() == JSONtype::NIL) {
			actionData_.preferencesJson = nullptr;
			return;
		}
		action_ = ActionType::LOADING;
		process();
		actionData_.preferencesJson = nullptr;
	}

	/*!
	* \brief Loads the object from a JSON file
	* \param The name of the JSON file
	*
	* \note It calls the overloaded process() method
	* \note If the file cannot be read, nothing is done
	* \note Not only that it's not thread-safe, it's not even reentrant
	*/
	inline void load(const std::string& fileName) {
		std::shared_ptr<JSON> target = parseJSON(fileName);
		actionData_.preferencesJson = target.get();
		if (actionData_.preferencesJson->type() == JSONtype::NIL) {
			actionData_.preferencesJson = nullptr;
			return;
		}
		action_ = ActionType::LOADING;
		process();
		actionData_.preferencesJson = nullptr;
	}

	/*!
	* \brief Overload this to change the behaviour of all GUI construction of this class, while it appears in the tree
	* and retains all other functionality
	* \param The layout to fill
	* \param Optional offset down
	* \param Optional offset left
	* \param Optional functor providing callback when anything is changed
	*
	* \note Use setupProcess() to set up the behaviour of the synch() calls
	* \note Using the process() method is not obligatory, but it must be present for serialisation anyway
	* \note You can use the action() method to check if the GUI is to be laid out normally or horizontally
	*/
	inline virtual void constructGUI(QGridLayout* layout, int gridDown, int gridRight, std::shared_ptr<std::function<void()>> callback) {
		setupProcess(layout, gridDown, gridRight, callback);
		process();
	}

	/*!
	* \brief Fills a QGridLayout using the class, can be overloaded to change its behaviour
	* \param The layout to fill
	* \param Optional offset down
	* \param Optional offset left
	* \param Optional functor providing callback when anything is changed
	*
	* \note Overloading this will change the behaviour of all other makeGUI calls
	* \note It calls the overloaded process() method
	* \note Not only that it's not thread-safe, it's not even reentrant
	*/
	inline void makeGUI(QGridLayout* layout, int gridDown = 0, int gridRight = 0, std::shared_ptr<std::function<void()>> callback = nullptr) {
		action_ = ActionType::GUI;
		std::unique_ptr<GUImakingInfo> info(new GUImakingInfo());
		actionData_.guiInfo = info.get();
		constructGUI(layout, gridDown, gridRight, callback);
		actionData_.guiInfo = nullptr;
	}

	/*!
	* \brief Fills a line of a QGridLayout table using the class, can be overloaded to change its behaviour
	* \param The layout to fill
	* \param Optional offset down
	* \param Optional offset left
	* \param Optional functor providing callback when anything is changed
	*
	* \note Overloading this will change the behaviour of all other makeGUI calls
	* \note It calls the overloaded process() method
	* \note Not only that it's not thread-safe, it's not even reentrant
	*/
	inline void makeGUItable(QGridLayout* layout, int gridDown = 0, int gridRight = 0, std::shared_ptr<std::function<void()>> callback = nullptr) {
		action_ = ActionType::GUItable;
		std::unique_ptr<GUImakingInfo> info(new GUImakingInfo());
		actionData_.guiInfo = info.get();
		constructGUI(layout, gridDown, gridRight, callback);
		actionData_.guiInfo = nullptr;
	}

	/*!
	* \brief Fills a QGridLayout using the class
	* \param The layout to fill
	* \param Optional functor providing callback when anything is changed
	*
	* \note It calls the overloaded process() method
	* \note Not only that it's not thread-safe, it's not even reentrant
	*/
	inline void makeGUI(QGridLayout* layout, int gridDown = 0, int gridRight = 0, std::function<void()> callback = nullptr) {
		makeGUI(layout, gridDown, gridRight, callback ? std::make_shared<std::function<void()>>(callback) : nullptr);
	}

	/*!
	* \brief Generates a Qt widget from the class
	* \param Optional functor providing callback when anything is changed
	*
	* \note It calls the overloaded process() method
	* \note Not only that it's not thread-safe, it's not even reentrant
	*/
	inline QWidget* makeGUI(std::function<void()> callback = nullptr) {
		return makeGUI(callback ? std::make_shared<std::function<void()>>(callback) : nullptr);
	}

	/*!
	* \brief Generates a Qt widget from the class
	* \param Optional functor providing callback when anything is changed
	*
	* \note It calls the overloaded process() method
	* \note Not only that it's not thread-safe, it's not even reentrant
	*/
	inline QWidget* makeGUI(std::shared_ptr<std::function<void()>> callback = nullptr) {
		std::unique_ptr<QWidget> retval(new QWidget);
		std::unique_ptr<QGridLayout> layout(new QGridLayout());
		makeGUI(layout.get(), 0, 0, callback);
		retval->setLayout(layout.release());
		return retval.release();
	}
};

#endif //QUICK_PREFERENCES_HPP
