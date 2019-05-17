# Quick Preferences
A small header-only library for convenient setting and saving of program preferences without boilerplate and for generating Qt-based UIs that allow modifying the stucture by the user. It uses the [JSON format](https://en.wikipedia.org/wiki/JSON). It's designed for minising the amount of code used for saving and loading while keeping the saved file human-readable, it's not optimised for performance.

Normally, persistent preferences tend to have one method for saving that contains code for loading all the content and one method for saving it all. Also, there might be some validity checking. This allows saving it all in one method that calls overloads of one method to all members to be saved.

It contains a small JSON library to avoid a dependency on a library that is probably much larger than this one. It's not advised to be used for generic JSON usage.

A fork from an earlier version without the ability to generate UIs and without the dependency on Qt is [here](https://github.com/Dugy/serialisable).

## Usage

Have your classes inherit from the QuickPreferences class. They have to implement a `saveOrLoad()` method that calls overloads of the `synch()` method that accepts name of the value in the file as first argument and the value (taken as reference) as the second one. If something needs to be processed before saving or after loading, the `saving()` method will return a bool value telling if it's being saved.

Supported types are `std::string`, arithmetic types (converted to `double` because of JSON's specifications), `bool`, any object derived from `QuickPreferences`, a `std::vector` of such objects or a `std::vector` of smart pointers to such objects (raw pointers will not be deleted if `load()` is called while the vector is not empty).

Default values should be set somewhere, because if `load()` does not find the specified file, it does not call the `saveOrLoad()` method.

Missing keys will simply not write any value. Values of wrong types will throw.

```C++
struct Chapter : public QuickPreferences {
	std::string contents = "";
	std::string author = "Anonymous";

	virtual void saveOrLoad() {
		synch("contents", contents);
		synch("author", author);
	}
};

struct Preferences : public QuickPreferences {
	std::string lastFolder = "";
	unsigned int lastOpen = 0;
	bool privileged = false;
	Chapter info;
	std::vector<Chapter> chapters;
	std::vector<std::shared_ptr<Chapter>> footnotes;
	std::vector<std::unique_ptr<Chapter>> addenda;

	virtual void saveOrLoad() {
		synch("last_folder", lastFolder);
		synch("last_open", lastOpen);
		synch("privileged", privileged);
		synch("info", info);
		synch("chapters", chapters);
		synch("footnotes", footnotes);
		synch("addenda", addenda);
	}
};

//...

// Loading
Preferences prefs;
prefs.load("prefs.json");
  
// Saving
prefs.save("prefs.json");
```

To generate a UI, all you need is this:
```C++
 	// Assuming preferences_ is a class that inherits from QuickPreferences
	QWidget* made = preferences_.makeGUI(); // Create the widget
	// Supposing ui->mainLayout is a layout
	ui->mainLayout->addWidget(made); // Add it to a window
```

The `makeGUI()` method can have an occasional parameter of type `std:function<void()>` that is called after any change.

It relies on Qt widget libraries and C++11 standard libraries. It does not need the meta object compiler or any modifications to C++ occasionally used by Qt. For a version without dependencies on Qt, use [this fork](https://github.com/Dugy/serialisable).

To customise how an object in the tree structure builds its widgets, you can overload a method called constructGUI(), where you may or may not use the process() method that you must have defined anyway. This code for example postpones the callback until the _Accept_ button is pressed:
```C++
virtual void constructGUI(QGridLayout* layout, int gridDown, int gridRight, std::shared_ptr<std::function<void()>> callback) {
	QPushButton* accept = new QPushButton("Accept");
	accept->setEnabled(false);
	setupProcess(layout, gridDown, gridRight, std::make_shared<std::function<void()>>([accept] () {
		accept->setEnabled(true);
	}));
	process();
	layout->addWidget(accept, layout->rowCount(), 0, 1, layout->columnCount());
	QObject::connect(accept, &QPushButton::clicked, accept, [callback, accept] () {
		accept->setEnabled(false);
		(*callback)();
	});
}
```

## JSON library

The JSON library provided is only to avoid having additional dependencies. It's written to be short, its usage is prone to result in repetitive code. If you need JSON for something else, use a proper JSON library, like [the one written by Niels Lohmann](https://github.com/nlohmann/json), they are much more convenient.

If you really need to use it, for example if you are sure you will not use it much, here is an example:

``` C++
QuickPreferences::JSONobject testJson;
testJson.getObject()["file"] = std::make_shared<QuickPreferences::JSONstring>("test.json");
testJson.getObject()["number"] = std::make_shared<QuickPreferences::JSONdouble>(9);
testJson.getObject()["makes_sense"] = std::make_shared<QuickPreferences::JSONbool>(false);
std::shared_ptr<QuickPreferences::JSONarray> array = std::make_shared<QuickPreferences::JSONarray>();
for (int i = 0; i < 3; i++) {
	std::shared_ptr<QuickPreferences::JSONobject> obj = std::make_shared<QuickPreferences::JSONobject>();
	obj->getObject()["index"] = std::make_shared<QuickPreferences::JSONdouble>(i);
	std::shared_ptr<QuickPreferences::JSONobject> obj2 = std::make_shared<QuickPreferences::JSONobject>();
	obj->getObject()["contents"] = obj2;
	obj2->getObject()["empty"] = std::make_shared<QuickPreferences::JSONobject>();
	array->getVector().push_back(obj);
}
testJson.getObject()["data"] = array;
testJson.writeToFile("test.json");

std::shared_ptr<QuickPreferences::JSON> testReadJson = QuickPreferences::parseJSON("test.json");
testReadJson->getObject()["makes_sense"]->getBool() = true;
testReadJson->getObject()["number"]->getDouble() = 42;
testReadJson->writeToFile("test-reread.json");
```

The structure consists of JSON nodes of various types. They all have the same methods for accessing the contents returning references to the correct types (`getString()`, `getDouble()`, `getBool()`, `getObject()` and `getArray()`), but they are all virtual and only the correct one will not throw an exception. The type can be learned using the `type()` method. The interface class `QuickPreferences::JSON` is also the _null_ type.

The parser can parse incorrect code in some cases because some of the information in JSON files is redundant.

## TODO

* Allow supplying flags to the `synch()` method to optionally enable tabs bars instead of linear layouts, disable table generation, grouping of elements into more columns, some serialisation specifications or something else if that comes to my mind
