/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009 Jan Michael Alonzo
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "AccessibilityUIElement.h"

#if HAVE(ACCESSIBILITY)

#include "AccessibilityNotificationHandlerAtk.h"
#include <JavaScriptCore/JSStringRef.h>
#include <JavaScriptCore/OpaqueJSString.h>
#include <atk/atk.h>
#include <wtf/Assertions.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>
#include <wtf/unicode/CharacterNames.h>

static String coreAttributeToAtkAttribute(JSStringRef attribute)
{
    size_t bufferSize = JSStringGetMaximumUTF8CStringSize(attribute);
    GOwnPtr<gchar> buffer(static_cast<gchar*>(g_malloc(bufferSize)));
    JSStringGetUTF8CString(attribute, buffer.get(), bufferSize);

    String attributeString = String::fromUTF8(buffer.get());
    if (attributeString == "AXInvalid")
        return "aria-invalid";

    if (attributeString == "AXPlaceholderValue")
        return "placeholder-text";

    if (attributeString == "AXSortDirection")
        return "aria-sort";

    return String();
}

static String getAttributeSetValueForId(AtkObject* accessible, const char* id)
{
    const char* attributeValue = 0;
    AtkAttributeSet* attributeSet = atk_object_get_attributes(accessible);
    for (AtkAttributeSet* attributes = attributeSet; attributes; attributes = attributes->next) {
        AtkAttribute* atkAttribute = static_cast<AtkAttribute*>(attributes->data);
        if (!strcmp(atkAttribute->name, id)) {
            attributeValue = atkAttribute->value;
            break;
        }
    }

    String atkAttributeValue = String::fromUTF8(attributeValue);
    atk_attribute_set_free(attributeSet);

    return atkAttributeValue;
}

static String getAtkAttributeSetAsString(AtkObject* accessible)
{
    StringBuilder builder;

    AtkAttributeSet* attributeSet = atk_object_get_attributes(accessible);
    for (AtkAttributeSet* attributes = attributeSet; attributes; attributes = attributes->next) {
        AtkAttribute* attribute = static_cast<AtkAttribute*>(attributes->data);
        GOwnPtr<gchar> attributeData(g_strconcat(attribute->name, ":", attribute->value, NULL));
        builder.append(attributeData.get());
        if (attributes->next)
            builder.append(", ");
    }
    atk_attribute_set_free(attributeSet);

    return builder.toString();
}

static inline const char* roleToString(AtkRole role)
{
    switch (role) {
    case ATK_ROLE_ALERT:
        return "AXAlert";
    case ATK_ROLE_CANVAS:
        return "AXCanvas";
    case ATK_ROLE_CHECK_BOX:
        return "AXCheckBox";
    case ATK_ROLE_COLOR_CHOOSER:
        return "AXColorWell";
    case ATK_ROLE_COLUMN_HEADER:
        return "AXColumnHeader";
    case ATK_ROLE_COMBO_BOX:
        return "AXComboBox";
    case ATK_ROLE_DOCUMENT_FRAME:
        return "AXWebArea";
    case ATK_ROLE_ENTRY:
        return "AXTextField";
    case ATK_ROLE_FOOTER:
        return "AXFooter";
    case ATK_ROLE_FORM:
        return "AXForm";
    case ATK_ROLE_GROUPING:
        return "AXGroup";
    case ATK_ROLE_HEADING:
        return "AXHeading";
    case ATK_ROLE_IMAGE:
        return "AXImage";
    case ATK_ROLE_IMAGE_MAP:
        return "AXImageMap";
    case ATK_ROLE_LABEL:
        return "AXLabel";
    case ATK_ROLE_LINK:
        return "AXLink";
    case ATK_ROLE_LIST:
        return "AXList";
    case ATK_ROLE_LIST_BOX:
        return "AXListBox";
    case ATK_ROLE_LIST_ITEM:
        return "AXListItem";
    case ATK_ROLE_MENU:
        return "AXMenu";
    case ATK_ROLE_MENU_BAR:
        return "AXMenuBar";
    case ATK_ROLE_MENU_ITEM:
        return "AXMenuItem";
    case ATK_ROLE_PAGE_TAB:
        return "AXTab";
    case ATK_ROLE_PAGE_TAB_LIST:
        return "AXTabGroup";
    case ATK_ROLE_PANEL:
        return "AXGroup";
    case ATK_ROLE_PARAGRAPH:
        return "AXParagraph";
    case ATK_ROLE_PASSWORD_TEXT:
        return "AXPasswordField";
    case ATK_ROLE_PUSH_BUTTON:
        return "AXButton";
    case ATK_ROLE_RADIO_BUTTON:
        return "AXRadioButton";
    case ATK_ROLE_ROW_HEADER:
        return "AXRowHeader";
    case ATK_ROLE_RULER:
        return "AXRuler";
    case ATK_ROLE_SCROLL_BAR:
        return "AXScrollBar";
    case ATK_ROLE_SCROLL_PANE:
        return "AXScrollArea";
    case ATK_ROLE_SECTION:
        return "AXDiv";
    case ATK_ROLE_SEPARATOR:
        return "AXHorizontalRule";
    case ATK_ROLE_SLIDER:
        return "AXSlider";
    case ATK_ROLE_SPIN_BUTTON:
        return "AXSpinButton";
    case ATK_ROLE_TABLE:
        return "AXTable";
    case ATK_ROLE_TABLE_CELL:
        return "AXCell";
    case ATK_ROLE_TABLE_COLUMN_HEADER:
        return "AXColumnHeader";
    case ATK_ROLE_TABLE_ROW:
        return "AXRow";
    case ATK_ROLE_TABLE_ROW_HEADER:
        return "AXRowHeader";
    case ATK_ROLE_TOGGLE_BUTTON:
        return "AXToggleButton";
    case ATK_ROLE_TOOL_BAR:
        return "AXToolbar";
    case ATK_ROLE_TOOL_TIP:
        return "AXUserInterfaceTooltip";
    case ATK_ROLE_TREE:
        return "AXTree";
    case ATK_ROLE_TREE_TABLE:
        return "AXTreeGrid";
    case ATK_ROLE_TREE_ITEM:
        return "AXTreeItem";
    case ATK_ROLE_WINDOW:
        return "AXWindow";
    case ATK_ROLE_UNKNOWN:
        return "AXUnknown";
    default:
        // We want to distinguish ATK_ROLE_UNKNOWN from a known AtkRole which
        // our DRT isn't properly handling.
        return "FIXME not identified";
    }
}

static inline gchar* replaceCharactersForResults(gchar* str)
{
    String uString = String::fromUTF8(str);

    // The object replacement character is passed along to ATs so we need to be
    // able to test for their presence and do so without causing test failures.
    uString.replace(objectReplacementCharacter, "<obj>");

    // The presence of newline characters in accessible text of a single object
    // is appropriate, but it makes test results (especially the accessible tree)
    // harder to read.
    uString.replace("\n", "<\\n>");

    return g_strdup(uString.utf8().data());
}

static bool checkElementState(PlatformUIElement element, AtkStateType stateType)
{
    if (!ATK_IS_OBJECT(element))
        return false;

    GRefPtr<AtkStateSet> stateSet = adoptGRef(atk_object_ref_state_set(ATK_OBJECT(element)));
    return atk_state_set_contains_state(stateSet.get(), stateType);
}

static String attributesOfElement(AccessibilityUIElement* element)
{
    StringBuilder builder;

    builder.append(String::format("%s\n", element->role()->string().utf8().data()));

    // For the parent we print its role and its name, if available.
    builder.append("AXParent: ");
    AccessibilityUIElement parent = element->parentElement();
    if (AtkObject* atkParent = parent.platformUIElement()) {
        builder.append(roleToString(atk_object_get_role(atkParent)));
        const char* parentName = atk_object_get_name(atkParent);
        if (parentName && g_utf8_strlen(parentName, -1))
            builder.append(String::format(": %s", parentName));
    } else
        builder.append("(null)");
    builder.append("\n");

    builder.append(String::format("AXChildren: %d\n", element->childrenCount()));
    builder.append(String::format("AXPosition: { %f, %f }\n", element->x(), element->y()));
    builder.append(String::format("AXSize: { %f, %f }\n", element->width(), element->height()));

    String title = element->title()->string();
    if (!title.isEmpty())
        builder.append(String::format("%s\n", title.utf8().data()));

    String description = element->description()->string();
    if (!description.isEmpty())
        builder.append(String::format("%s\n", description.utf8().data()));

    String value = element->stringValue()->string();
    if (!value.isEmpty())
        builder.append(String::format("%s\n", value.utf8().data()));

    builder.append(String::format("AXFocusable: %d\n", element->isFocusable()));
    builder.append(String::format("AXFocused: %d\n", element->isFocused()));
    builder.append(String::format("AXSelectable: %d\n", element->isSelectable()));
    builder.append(String::format("AXSelected: %d\n", element->isSelected()));
    builder.append(String::format("AXMultiSelectable: %d\n", element->isMultiSelectable()));
    builder.append(String::format("AXEnabled: %d\n", element->isEnabled()));
    builder.append(String::format("AXExpanded: %d\n", element->isExpanded()));
    builder.append(String::format("AXRequired: %d\n", element->isRequired()));
    builder.append(String::format("AXChecked: %d\n", element->isChecked()));

    // We append the ATK specific attributes as a single line at the end.
    builder.append("AXPlatformAttributes: ");
    builder.append(getAtkAttributeSetAsString(element->platformUIElement()));

    return builder.toString();
}

AccessibilityUIElement::AccessibilityUIElement(PlatformUIElement element)
    : m_element(element)
{
    if (m_element)
        g_object_ref(m_element);
}

AccessibilityUIElement::AccessibilityUIElement(const AccessibilityUIElement& other)
    : m_element(other.m_element)
{
    if (m_element)
        g_object_ref(m_element);
}

AccessibilityUIElement::~AccessibilityUIElement()
{
    if (m_element)
        g_object_unref(m_element);
}

void AccessibilityUIElement::getLinkedUIElements(Vector<AccessibilityUIElement>& elements)
{
    // FIXME: implement
}

void AccessibilityUIElement::getDocumentLinks(Vector<AccessibilityUIElement>&)
{
    // FIXME: implement
}

void AccessibilityUIElement::getChildren(Vector<AccessibilityUIElement>& children)
{
    int count = childrenCount();
    for (int i = 0; i < count; i++) {
        AtkObject* child = atk_object_ref_accessible_child(ATK_OBJECT(m_element), i);
        children.append(AccessibilityUIElement(child));
    }
}

void AccessibilityUIElement::getChildrenWithRange(Vector<AccessibilityUIElement>& elementVector, unsigned start, unsigned end)
{
    for (unsigned i = start; i < end; i++) {
        AtkObject* child = atk_object_ref_accessible_child(ATK_OBJECT(m_element), i);
        elementVector.append(AccessibilityUIElement(child));
    }
}

int AccessibilityUIElement::rowCount()
{
    if (!m_element)
        return 0;

    ASSERT(ATK_IS_TABLE(m_element));

    return atk_table_get_n_rows(ATK_TABLE(m_element));
}

int AccessibilityUIElement::columnCount()
{
    if (!m_element)
        return 0;

    ASSERT(ATK_IS_TABLE(m_element));

    return atk_table_get_n_columns(ATK_TABLE(m_element));
}

int AccessibilityUIElement::childrenCount()
{
    if (!m_element)
        return 0;

    ASSERT(ATK_IS_OBJECT(m_element));

    return atk_object_get_n_accessible_children(ATK_OBJECT(m_element));
}

AccessibilityUIElement AccessibilityUIElement::elementAtPoint(int x, int y)
{
    if (!m_element)
        return 0;

    return AccessibilityUIElement(atk_component_ref_accessible_at_point(ATK_COMPONENT(m_element), x, y, ATK_XY_WINDOW));
}

AccessibilityUIElement AccessibilityUIElement::linkedUIElementAtIndex(unsigned index)
{
    // FIXME: implement
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::getChildAtIndex(unsigned index)
{
    Vector<AccessibilityUIElement> children;
    getChildrenWithRange(children, index, index + 1);

    if (children.size() == 1)
        return children.at(0);

    return 0;
}

unsigned AccessibilityUIElement::indexOfChild(AccessibilityUIElement* element)
{ 
    // FIXME: implement
    return 0;
}

JSStringRef AccessibilityUIElement::allAttributes()
{
    if (!m_element || !ATK_IS_OBJECT(m_element))
        return JSStringCreateWithCharacters(0, 0);

    return JSStringCreateWithUTF8CString(attributesOfElement(this).utf8().data());
}

JSStringRef AccessibilityUIElement::attributesOfLinkedUIElements()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfDocumentLinks()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

AccessibilityUIElement AccessibilityUIElement::titleUIElement()
{
    if (!m_element)
        return 0;

    AtkRelationSet* set = atk_object_ref_relation_set(ATK_OBJECT(m_element));
    if (!set)
        return 0;

    AtkObject* target = 0;
    int count = atk_relation_set_get_n_relations(set);
    for (int i = 0; i < count; i++) {
        AtkRelation* relation = atk_relation_set_get_relation(set, i);
        if (atk_relation_get_relation_type(relation) == ATK_RELATION_LABELLED_BY) {
            GPtrArray* targetList = atk_relation_get_target(relation);
            if (targetList->len)
                target = static_cast<AtkObject*>(g_ptr_array_index(targetList, 0));
        }
    }

    g_object_unref(set);
    return target ? AccessibilityUIElement(target) : 0;
}

AccessibilityUIElement AccessibilityUIElement::parentElement()
{
    if (!m_element)
        return 0;

    ASSERT(ATK_IS_OBJECT(m_element));

    AtkObject* parent =  atk_object_get_parent(ATK_OBJECT(m_element));
    return parent ? AccessibilityUIElement(parent) : 0;
}

JSStringRef AccessibilityUIElement::attributesOfChildren()
{
    Vector<AccessibilityUIElement> children;
    getChildren(children);

    StringBuilder builder;
    for (Vector<AccessibilityUIElement>::iterator it = children.begin(); it != children.end(); ++it) {
        builder.append(attributesOfElement(it));
        builder.append("\n------------\n");
    }

    return JSStringCreateWithUTF8CString(builder.toString().utf8().data());
}

JSStringRef AccessibilityUIElement::parameterizedAttributeNames()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::role()
{
    if (!m_element || !ATK_IS_OBJECT(m_element))
        return JSStringCreateWithCharacters(0, 0);

    AtkRole role = atk_object_get_role(ATK_OBJECT(m_element));
    if (!role)
        return JSStringCreateWithCharacters(0, 0);

    GOwnPtr<char> roleStringWithPrefix(g_strdup_printf("AXRole: %s", roleToString(role)));
    return JSStringCreateWithUTF8CString(roleStringWithPrefix.get());
}

JSStringRef AccessibilityUIElement::subrole()
{
    return 0;
}

JSStringRef AccessibilityUIElement::roleDescription()
{
    return 0;
}

JSStringRef AccessibilityUIElement::title()
{
    const gchar* name = atk_object_get_name(ATK_OBJECT(m_element));
    GOwnPtr<gchar> axTitle(g_strdup_printf("AXTitle: %s", name ? name : ""));

    return JSStringCreateWithUTF8CString(axTitle.get());
}

JSStringRef AccessibilityUIElement::description()
{
    const gchar* description = atk_object_get_description(ATK_OBJECT(m_element));

    if (!description)
        return JSStringCreateWithCharacters(0, 0);

    GOwnPtr<gchar> axDesc(g_strdup_printf("AXDescription: %s", description));

    return JSStringCreateWithUTF8CString(axDesc.get());
}

JSStringRef AccessibilityUIElement::stringValue()
{
    if (!m_element || !ATK_IS_TEXT(m_element))
        return JSStringCreateWithCharacters(0, 0);

    GOwnPtr<gchar> text(atk_text_get_text(ATK_TEXT(m_element), 0, -1));
    GOwnPtr<gchar> textWithReplacedCharacters(replaceCharactersForResults(text.get()));
    GOwnPtr<gchar> axValue(g_strdup_printf("AXValue: %s", textWithReplacedCharacters.get()));

    return JSStringCreateWithUTF8CString(axValue.get());
}

JSStringRef AccessibilityUIElement::language()
{
    if (!m_element)
        return JSStringCreateWithCharacters(0, 0);

    const gchar* locale = atk_object_get_object_locale(ATK_OBJECT(m_element));
    if (!locale)
        return JSStringCreateWithCharacters(0, 0);

    GOwnPtr<char> axValue(g_strdup_printf("AXLanguage: %s", locale));
    return JSStringCreateWithUTF8CString(axValue.get());
}

double AccessibilityUIElement::x()
{
    int x, y;

    atk_component_get_position(ATK_COMPONENT(m_element), &x, &y, ATK_XY_SCREEN);

    return x;
}

double AccessibilityUIElement::y()
{
    int x, y;

    atk_component_get_position(ATK_COMPONENT(m_element), &x, &y, ATK_XY_SCREEN);

    return y;
}

double AccessibilityUIElement::width()
{
    int width, height;

    atk_component_get_size(ATK_COMPONENT(m_element), &width, &height);

    return width;
}

double AccessibilityUIElement::height()
{
    int width, height;

    atk_component_get_size(ATK_COMPONENT(m_element), &width, &height);

    return height;
}

double AccessibilityUIElement::clickPointX()
{
    // Note: This is not something we have in ATK.
    return 0.f;
}

double AccessibilityUIElement::clickPointY()
{
    // Note: This is not something we have in ATK.
    return 0.f;
}

JSStringRef AccessibilityUIElement::orientation() const
{
    if (!m_element || !ATK_IS_OBJECT(m_element))
        return JSStringCreateWithCharacters(0, 0);

    const char* axOrientation = 0;
    if (checkElementState(m_element, ATK_STATE_HORIZONTAL))
        axOrientation = "AXOrientation: AXHorizontalOrientation";
    else if (checkElementState(m_element, ATK_STATE_VERTICAL))
        axOrientation = "AXOrientation: AXVerticalOrientation";

    if (!axOrientation)
        return JSStringCreateWithCharacters(0, 0);

    return JSStringCreateWithUTF8CString(axOrientation);
}

double AccessibilityUIElement::intValue() const
{
    GValue value = { 0, { { 0 } } };

    if (!ATK_IS_VALUE(m_element))
        return 0.0f;

    atk_value_get_current_value(ATK_VALUE(m_element), &value);
    if (!G_VALUE_HOLDS_FLOAT(&value))
        return 0.0f;
    return g_value_get_float(&value);
}

double AccessibilityUIElement::minValue()
{
    GValue value = { 0, { { 0 } } };

    if (!ATK_IS_VALUE(m_element))
        return 0.0f;

    atk_value_get_minimum_value(ATK_VALUE(m_element), &value);
    if (!G_VALUE_HOLDS_FLOAT(&value))
        return 0.0f;
    return g_value_get_float(&value);
}

double AccessibilityUIElement::maxValue()
{
    GValue value = { 0, { { 0 } } };

    if (!ATK_IS_VALUE(m_element))
        return 0.0f;

    atk_value_get_maximum_value(ATK_VALUE(m_element), &value);
    if (!G_VALUE_HOLDS_FLOAT(&value))
        return 0.0f;
    return g_value_get_float(&value);
}

JSStringRef AccessibilityUIElement::valueDescription()
{
    // FIXME: implement after it has been implemented in ATK.
    // See: https://bugzilla.gnome.org/show_bug.cgi?id=684576
    return JSStringCreateWithCharacters(0, 0);
}

bool AccessibilityUIElement::isEnabled()
{
    return checkElementState(m_element, ATK_STATE_ENABLED);
}

int AccessibilityUIElement::insertionPointLineNumber()
{
    // FIXME: implement
    return 0;
}

bool AccessibilityUIElement::isPressActionSupported()
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isIncrementActionSupported()
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isDecrementActionSupported()
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isRequired() const
{
    return checkElementState(m_element, ATK_STATE_REQUIRED);
}

bool AccessibilityUIElement::isFocused() const
{
    if (!ATK_IS_OBJECT(m_element))
        return false;

    GRefPtr<AtkStateSet> stateSet = adoptGRef(atk_object_ref_state_set(ATK_OBJECT(m_element)));
    gboolean isFocused = atk_state_set_contains_state(stateSet.get(), ATK_STATE_FOCUSED);

    return isFocused;
}

bool AccessibilityUIElement::isSelected() const
{
    return checkElementState(m_element, ATK_STATE_SELECTED);
}

int AccessibilityUIElement::hierarchicalLevel() const
{
    // FIXME: implement
    return 0;
}

bool AccessibilityUIElement::ariaIsGrabbed() const
{
    return false;
}

JSStringRef AccessibilityUIElement::ariaDropEffects() const
{   
    return 0; 
}

bool AccessibilityUIElement::isExpanded() const
{
    if (!ATK_IS_OBJECT(m_element))
        return false;

    GRefPtr<AtkStateSet> stateSet = adoptGRef(atk_object_ref_state_set(ATK_OBJECT(m_element)));
    gboolean isExpanded = atk_state_set_contains_state(stateSet.get(), ATK_STATE_EXPANDED);

    return isExpanded;
}

bool AccessibilityUIElement::isChecked() const
{
    if (!ATK_IS_OBJECT(m_element))
        return false;

    GRefPtr<AtkStateSet> stateSet = adoptGRef(atk_object_ref_state_set(ATK_OBJECT(m_element)));
    gboolean isChecked = atk_state_set_contains_state(stateSet.get(), ATK_STATE_CHECKED);

    return isChecked;
}

JSStringRef AccessibilityUIElement::attributesOfColumnHeaders()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfRowHeaders()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfColumns()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfRows()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfVisibleCells()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::attributesOfHeader()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

int AccessibilityUIElement::indexInTable()
{
    // FIXME: implement
    return 0;
}

static JSStringRef indexRangeInTable(PlatformUIElement element, bool isRowRange)
{
    GOwnPtr<gchar> rangeString(g_strdup("{0, 0}"));

    if (!element)
        return JSStringCreateWithUTF8CString(rangeString.get());

    ASSERT(ATK_IS_OBJECT(element));

    AtkObject* axTable = atk_object_get_parent(ATK_OBJECT(element));
    if (!axTable || !ATK_IS_TABLE(axTable))
        return JSStringCreateWithUTF8CString(rangeString.get());

    // Look for the cell in the table.
    gint indexInParent = atk_object_get_index_in_parent(ATK_OBJECT(element));
    if (indexInParent == -1)
        return JSStringCreateWithUTF8CString(rangeString.get());

    int row = -1;
    int column = -1;
    row = atk_table_get_row_at_index(ATK_TABLE(axTable), indexInParent);
    column = atk_table_get_column_at_index(ATK_TABLE(axTable), indexInParent);

    // Get the actual values, if row and columns are valid values.
    if (row != -1 && column != -1) {
        int base = 0;
        int length = 0;
        if (isRowRange) {
            base = row;
            length = atk_table_get_row_extent_at(ATK_TABLE(axTable), row, column);
        } else {
            base = column;
            length = atk_table_get_column_extent_at(ATK_TABLE(axTable), row, column);
        }
        rangeString.set(g_strdup_printf("{%d, %d}", base, length));
    }

    return JSStringCreateWithUTF8CString(rangeString.get());
}

JSStringRef AccessibilityUIElement::rowIndexRange()
{
    // Range in table for rows.
    return indexRangeInTable(m_element, true);
}

JSStringRef AccessibilityUIElement::columnIndexRange()
{
    // Range in table for columns.
    return indexRangeInTable(m_element, false);
}

int AccessibilityUIElement::lineForIndex(int)
{
    // FIXME: implement
    return 0;
}

JSStringRef AccessibilityUIElement::boundsForRange(unsigned location, unsigned length)
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::stringForRange(unsigned, unsigned) 
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
} 

JSStringRef AccessibilityUIElement::attributedStringForRange(unsigned, unsigned)
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

bool AccessibilityUIElement::attributedStringRangeIsMisspelled(unsigned location, unsigned length)
{
    // FIXME: implement
    return false;
}

AccessibilityUIElement AccessibilityUIElement::uiElementForSearchPredicate(JSContextRef context, AccessibilityUIElement* startElement, bool isDirectionNext, JSValueRef searchKey, JSStringRef searchText, bool visibleOnly)
{
    // FIXME: implement
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::cellForColumnAndRow(unsigned column, unsigned row)
{
    if (!m_element)
        return 0;

    ASSERT(ATK_IS_TABLE(m_element));

    // Adopt the AtkObject representing the cell because
    // at_table_ref_at() transfers full ownership.
    GRefPtr<AtkObject> foundCell = adoptGRef(atk_table_ref_at(ATK_TABLE(m_element), row, column));
    return foundCell ? AccessibilityUIElement(foundCell.get()) : 0;
}

JSStringRef AccessibilityUIElement::selectedTextRange()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

void AccessibilityUIElement::setSelectedTextRange(unsigned location, unsigned length)
{
    // FIXME: implement
}

JSStringRef AccessibilityUIElement::stringAttributeValue(JSStringRef attribute)
{
    if (!m_element)
        return JSStringCreateWithCharacters(0, 0);

    String atkAttributeName = coreAttributeToAtkAttribute(attribute);
    if (atkAttributeName.isEmpty())
        return JSStringCreateWithCharacters(0, 0);

    String attributeValue = getAttributeSetValueForId(ATK_OBJECT(m_element), atkAttributeName.utf8().data());

    // In case of 'aria-invalid' when the attribute empty or has "false" for ATK
    // according to http://www.w3.org/WAI/PF/aria-implementation/#mapping attribute
    // is not mapped but layout tests will expect 'false'.
    if (attributeValue.isEmpty() && atkAttributeName == "aria-invalid")
        return JSStringCreateWithUTF8CString("false");

    return JSStringCreateWithUTF8CString(attributeValue.utf8().data());
}

double AccessibilityUIElement::numberAttributeValue(JSStringRef attribute)
{
    // FIXME: implement
    return 0.0f;
}

bool AccessibilityUIElement::boolAttributeValue(JSStringRef attribute)
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isAttributeSettable(JSStringRef attribute)
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isAttributeSupported(JSStringRef attribute)
{
    return false;
}

static void alterCurrentValue(PlatformUIElement element, int factor)
{
    if (!element)
        return;

    ASSERT(ATK_IS_VALUE(element));

    GValue currentValue = G_VALUE_INIT;
    atk_value_get_current_value(ATK_VALUE(element), &currentValue);

    GValue increment = G_VALUE_INIT;
    atk_value_get_minimum_increment(ATK_VALUE(element), &increment);

    GValue newValue = G_VALUE_INIT;
    g_value_init(&newValue, G_TYPE_FLOAT);

    g_value_set_float(&newValue, g_value_get_float(&currentValue) + factor * g_value_get_float(&increment));
    atk_value_set_current_value(ATK_VALUE(element), &newValue);

    g_value_unset(&newValue);
    g_value_unset(&increment);
    g_value_unset(&currentValue);
}

void AccessibilityUIElement::increment()
{
    alterCurrentValue(m_element, 1);
}

void AccessibilityUIElement::decrement()
{
    alterCurrentValue(m_element, -1);
}

void AccessibilityUIElement::press()
{
    if (!m_element)
        return;

    ASSERT(ATK_IS_OBJECT(m_element));

    if (!ATK_IS_ACTION(m_element))
        return;

    // Only one action per object is supported so far.
    atk_action_do_action(ATK_ACTION(m_element), 0);
}

void AccessibilityUIElement::showMenu()
{
    // FIXME: implement
}

AccessibilityUIElement AccessibilityUIElement::disclosedRowAtIndex(unsigned index)
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::ariaOwnsElementAtIndex(unsigned index)
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::ariaFlowToElementAtIndex(unsigned index)
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::selectedRowAtIndex(unsigned index)
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::rowAtIndex(unsigned index)
{
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::disclosedByRow()
{
    return 0;
}

JSStringRef AccessibilityUIElement::accessibilityValue() const
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::documentEncoding()
{
    AtkRole role = atk_object_get_role(ATK_OBJECT(m_element));
    if (role != ATK_ROLE_DOCUMENT_FRAME)
        return JSStringCreateWithCharacters(0, 0);

    return JSStringCreateWithUTF8CString(atk_document_get_attribute_value(ATK_DOCUMENT(m_element), "Encoding"));
}

JSStringRef AccessibilityUIElement::documentURI()
{
    AtkRole role = atk_object_get_role(ATK_OBJECT(m_element));
    if (role != ATK_ROLE_DOCUMENT_FRAME)
        return JSStringCreateWithCharacters(0, 0);

    return JSStringCreateWithUTF8CString(atk_document_get_attribute_value(ATK_DOCUMENT(m_element), "URI"));
}

JSStringRef AccessibilityUIElement::url()
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

bool AccessibilityUIElement::addNotificationListener(JSObjectRef functionCallback)
{
    if (!functionCallback)
        return false;

    // Only one notification listener per element.
    if (m_notificationHandler)
        return false;

    m_notificationHandler = new AccessibilityNotificationHandler();
    m_notificationHandler->setPlatformElement(platformUIElement());
    m_notificationHandler->setNotificationFunctionCallback(functionCallback);

    return true;
}

void AccessibilityUIElement::removeNotificationListener()
{
    // Programmers should not be trying to remove a listener that's already removed.
    ASSERT(m_notificationHandler);

    m_notificationHandler = 0;
}

bool AccessibilityUIElement::isFocusable() const
{
    if (!ATK_IS_OBJECT(m_element))
        return false;

    GRefPtr<AtkStateSet> stateSet = adoptGRef(atk_object_ref_state_set(ATK_OBJECT(m_element)));
    gboolean isFocusable = atk_state_set_contains_state(stateSet.get(), ATK_STATE_FOCUSABLE);

    return isFocusable;
}

bool AccessibilityUIElement::isSelectable() const
{
    return checkElementState(m_element, ATK_STATE_SELECTABLE);
}

bool AccessibilityUIElement::isMultiSelectable() const
{
    return checkElementState(m_element, ATK_STATE_MULTISELECTABLE);
}

bool AccessibilityUIElement::isSelectedOptionActive() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isVisible() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isOffScreen() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isCollapsed() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isIgnored() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::hasPopup() const
{
    if (!m_element || !ATK_IS_OBJECT(m_element))
        return false;

    return equalIgnoringCase(getAttributeSetValueForId(ATK_OBJECT(m_element), "aria-haspopup"), "true");
}

void AccessibilityUIElement::takeFocus()
{
    // FIXME: implement
}

void AccessibilityUIElement::takeSelection()
{
    // FIXME: implement
}

void AccessibilityUIElement::addSelection()
{
    // FIXME: implement
}

void AccessibilityUIElement::removeSelection()
{
    // FIXME: implement
}

void AccessibilityUIElement::scrollToMakeVisible()
{
    // FIXME: implement
}

void AccessibilityUIElement::scrollToMakeVisibleWithSubFocus(int x, int y, int width, int height)
{
    // FIXME: implement
}

void AccessibilityUIElement::scrollToGlobalPoint(int x, int y)
{
    // FIXME: implement
}

#endif
