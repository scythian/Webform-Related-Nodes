<?php

/**
 * @file
 * Webform related nodes component.
 */

/**
 * Implements _webform_defaults_component().
 */
function _webform_defaults_related_nodes() {
  return array(
    'name' => '',
    'form_key' => NULL,
    'required' => 0,
    'pid' => 0,
    'weight' => 0,
    'value' => '',
    'extra' => array(
      'multiple' => NULL,
      'content_types' => array(),
      'private' => FALSE,
    ),
  );
}

/**
 * Implements _webform_theme_component().
 */
function _webform_theme_related_nodes() {
  return array(
    'webform_display_related_nodes' => array(
      'render element' => 'element',
    ),
  );
}

/**
 * Implements _webform_edit_component().
 */
function _webform_edit_related_nodes($component) {
  $form = array();

  // Prepare Content Type List.
  $content_type_options = array();
  $query = db_select('node_type', 'n');
  $query->fields('n', array('type', 'name'));
  $query->condition('n.module', 'node');
  $query->orderBy('n.name', 'ASC');
  $content_type_options = $query->execute()
    ->fetchAllKeyed();

  // Content type list.
  $form['extra']['content_types'] = array(
    '#title' => t('Related Content Type'),
    '#type' => 'checkboxes',
    '#options' => $content_type_options,
    '#default_value' => $component['extra']['content_types'],
    '#description' => t('Check needed content type(s) .'),
    '#required' => TRUE,
  );

  // Ability to set multiple answer.
  $form['extra']['multiple'] = array(
    '#type' => 'checkbox',
    '#title' => t('Multiple'),
    '#default_value' => $component['extra']['multiple'],
    '#description' => t('Check this option if the user should be allowed to choose multiple values.'),
    '#weight' => 0,
  );


  return $form;
}

/**
 * Implements _webform_render_component().
 */
function _webform_render_related_nodes($component, $value = NULL, $filter = TRUE) {
  $node = isset($component['nid']) ? node_load($component['nid']) : NULL;

  $element = array(
    '#title' => $filter ? webform_filter_xss($component['name']) : $component['name'],
    '#title_display' => $component['extra']['title_display'] ? $component['extra']['title_display'] : 'before',
    '#required' => $component['required'],
    '#weight' => $component['weight'],
    '#description' => $filter ? webform_filter_descriptions($component['extra']['description'], $node) : $component['extra']['description'],
    '#theme_wrappers' => array('webform_element'),
    '#pre_render' => array(),
    // Needed to disable double-wrapping of radios and checkboxes.
    '#translatable' => array('title', 'description', 'options'),
  );

  $default_value = $filter ? webform_replace_tokens($component['value'], $node) : $component['value'];

  $content_types = array_filter($component['extra']['content_types']);
  // Process options list of CT selected for component.
  $query = db_select('node', 'n');
  $query->fields('n', array('nid', 'title'));
  $query->condition('n.status', 1);
  $query->condition('n.type', $content_types);
  $query->orderBy('n.title', 'ASC');
  $options = $query->execute()
    ->fetchAllKeyed();

  if (empty($options)) {
    // Make element inaccessible if there are no options
    // as there is no point in showing it.
    $element['#access'] = FALSE;
  }
  // Set the component options.
  $element['#options'] = $options;

  // Add default options if using a select list with no default. This trigger's
  // Drupal 7's adding of the option for us. See @form_process_select().
  if (!$component['extra']['multiple'] && $default_value === '') {
    $element['#empty_value'] = '';
  }

  // Set the default value. Note: "No choice" is stored as an empty string,
  // which will match a 0 key for radios; NULL is used to avoid unintentional
  // defaulting to the 0 option.
  if (isset($value)) {
    if ($component['extra']['multiple']) {
      // Set the value as an array.
      $element['#default_value'] = array();
      foreach ((array) $value as $key => $option_value) {
        $element['#default_value'][] = $option_value === '' ? NULL : $option_value;
      }
    }
    else {
      // Set the value as a single string.
      $element['#default_value'] = '';
      foreach ((array) $value as $option_value) {
        $element['#default_value'] = $option_value === '' ? NULL : $option_value;
      }
    }
  }
  elseif ($default_value !== '') {
    // Convert default value to a list if necessary.
    if ($component['extra']['multiple']) {
      $varray = explode(',', $default_value);
      foreach ($varray as $key => $v) {
        $v = trim($v);
        if ($v !== '') {
          $element['#default_value'][] = $v;
        }
      }
    }
    else {
      $element['#default_value'] = $default_value;
    }
  }
  elseif ($component['extra']['multiple']) {
    $element['#default_value'] = array();
  }

  if ($component['extra']['multiple']) {
    // Set display as a checkbox set.
    $element['#type'] = 'checkboxes';
    $element['#theme_wrappers'] = array_merge(array('checkboxes'), $element['#theme_wrappers']);
    $element['#process'] = array_merge(element_info_property('checkboxes', '#process'), array('webform_expand_select_ids'));

    // Entirely replace the normal expand checkboxes with our custom version.
    // This helps render checkboxes in multipage forms.
    $process_key = array_search('form_process_checkboxes', $element['#process']);
    $element['#process'][$process_key] = 'webform_expand_checkboxes';
  }
  else {
    // Set display as a radio set.
    $element['#type'] = 'radios';
    $element['#theme_wrappers'] = array_merge(array('radios'), $element['#theme_wrappers']);
    $element['#process'] = array_merge(element_info_property('radios', '#process'), array('webform_expand_select_ids'));
  }

  return $element;
}

/**
 * Implements _webform_display_component().
 */
function _webform_display_related_nodes($component, $value, $format = 'html') {
  // Process data, to find submitted nodes titles.
  $submitted_nodes = array();
  if (!empty($value)) {
    $query = db_select('node', 'n');
    $query->fields('n', array('nid', 'title'));
    $query->condition('n.nid', $value);
    $query->orderBy('n.title', 'ASC');
    $submitted_nodes = $query->execute()
      ->fetchAllKeyed();
  }

  return array(
    '#title' => $component['name'],
    '#title_display' => $component['extra']['title_display'] ? $component['extra']['title_display'] : 'before',
    '#weight' => $component['weight'],
    '#multiple' => $component['extra']['multiple'],
    '#theme' => 'webform_display_related_nodes',
    '#theme_wrappers' => $format == 'html' ? array('webform_element') : array('webform_element_text'),
    '#format' => $format,
    '#value' => $submitted_nodes,
    '#translatable' => array('title', 'options'),
  );
}

/**
 * Format the text output for this component.
 */
function theme_webform_display_related_nodes($variables) {
  $element = $variables['element'];

  $items = array();
  if ($element['#multiple']) {
    foreach ((array) $element['#value'] as $option_value) {
      if ($option_value !== '') {
        $items[] = $element['#format'] == 'html' ? check_plain($option_value) : $option_value;
      }
    }
  }
  else {
    if (!empty($element['#value'])) {
      $value = array_shift($element['#value']);
      $items[] = $element['#format'] == 'html' ? check_plain($value) : $value;
    }
  }

  if ($element['#format'] == 'html') {
    $output = count($items) > 1 ? theme('item_list', array('items' => $items)) : (isset($items[0]) ? $items[0] : ' ');
  }
  else {
    if (count($items) > 1) {
      foreach ($items as $key => $item) {
        $items[$key] = ' - ' . $item;
      }
      $output = implode("\n", $items);
    }
    else {
      $output = isset($items[0]) ? $items[0] : ' ';
    }
  }

  return $output;
}
